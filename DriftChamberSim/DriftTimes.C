#include <iostream>
#include <fstream>
#include <filesystem>

#include <cstdlib>
#include <cmath>
#include <stdlib.h> 

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <TApplication.h>
#include "Math/Vector3D.h"

#include "Garfield/ComponentAnalyticField.hh"
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/DriftLineRKF.hh"
#include "Garfield/TrackHeed.hh"

#define _USE_MATH_DEFINES

using namespace Garfield;
using json = nlohmann::ordered_json;
using xyzVector = ROOT::Math::XYZVector;

// Analytic fallback response function
auto response = [](double t) {
    const double tau = 8.;
    if (t < 0) return 0.0;
    return (t / tau) * std::exp(-t / tau);
};

bool readTransferFunction(Sensor& sensor) {
    std::ifstream infile("mdt_elx_delta.txt", std::ios::in);
    if (!infile) {
        std::cerr << "Could not read delta response function. Using analytic fallback.\n";
        sensor.SetTransferFunction(response);
        return true; 
    }
    
    std::vector<double> times;
    std::vector<double> values;
    double t = 0., f = 0.;
    while (infile >> t >> f) {
        times.push_back(1.e3 * t); 
        values.push_back(f);
    }
    infile.close();
    
    if (!times.empty()) {
        sensor.SetTransferFunction(times, values);
    } else {
        sensor.SetTransferFunction(response);
    }
    return true;
}

// Read in the drift chamber geometry information
json readGeometryFile() {
    std::ifstream geomFile("geometry.json");

    if (!geomFile.is_open()) {
        std::cerr << "Error: Could not open geometry.json" << std::endl;
        return json::object();
    }

    try {
        json jsonData = json::parse(geomFile);
        geomFile.close();
        return jsonData;
    }
    catch (const json::parse_error& e) {
        std::cerr << "Error: JSON Parse,  " << e.what() << std::endl;
    }
    
    return json::object();
}

// Get the Garfield++ file names based on the chamber geometry info
std::pair<std::string, std::string> getFileNames(json geomInfo){
    json mixture = geomInfo["gas_volume"]["mixture"];
    
    std::string gasStr = "";
    std::string ionMobility = "IonMobility_";
    
    bool isFirst = true;
    int count = 1;
    
    for (const auto& [materialName, materialData] : mixture.items()) {
        json equation = materialData["equation"];
        int percent = static_cast<int>(materialData["percent"].get<double>() *100.);
        
        std::string mixtureStr = "";
        
        for (const auto& [curName, num] : equation.items()){
            int curNum = num.get<int>();
            mixtureStr += curName;
            
            if (curNum != 1){
                mixtureStr += std::to_string(curNum);
            }
            
            if (isFirst){
                ionMobility += curName + "+_" + curName + ".txt";
                isFirst = false;
            }
        }
        gasStr += mixtureStr + "_" + std::to_string(percent);
        if (count < mixture.size()){ gasStr += "_"; }
        count++;
    }
    gasStr += ".gas";
    
    std::cout << "Gas file: " << gasStr << std::endl;
    std::cout << "IonMobility file: " << ionMobility << std::endl;
    
    return std::make_pair(gasStr, ionMobility);
}

// Get the diffusion constants
std::pair<double, double> getDiffusion(Sensor* sensor, ComponentAnalyticField* cmp, MediumMagboltz* gas, xyzVector pos){
    
    double bx = 0.0, by = 0.0, bz = 0.0; // Tesla
    double ex = 0.0, ey = 0.0, ez = 0.0; // V/cm
    double v = 0.0;                      // V
    Medium* medium = nullptr;
    int status = 0;
        
    // Get the electric and magnetic fields
    sensor->ElectricField(pos.X(), pos.Y(), pos.Z(), ex, ey, ez, v, medium, status);
    cmp->MagneticField(pos.X(), pos.Y(), pos.Z(), bx, by, bz, status);
    
    // Get the diffusion values
    double dl = 0.0, dt = 0.0; // sqrt(cm)
    gas->ElectronDiffusion(ex, ey, ez, bx, by, bz, dl, dt);
    
    return std::make_pair(dl, dt);
}

int main(int argc, char* argv[]) {
    // Read information about the drift chamber geometry
    json geomInfo = readGeometryFile();
    if (!geomInfo.size()) return 1;
    std::pair<std::string, std::string> filenames = getFileNames(geomInfo);

    int app_argc = 1;
    TApplication app("app", &app_argc, argv);

    MediumMagboltz gas;
    
    // Define the gas properties from the corresponding gas file
    if (!std::filesystem::exists(filenames.first)){
        std::cerr << "Error: File does not exist at " << filenames.first << std::endl;
        return 1;
    }
     gas.LoadGasFile(filenames.first);
    
    // Define the ion mobility from the corresponding file
    auto installdir = std::getenv("GARFIELD_INSTALL");
    const std::string path = installdir;
    const std::string fullPath = path + "/share/Garfield/Data/" + filenames.second;
    
    if (!installdir){
        std::cerr << "Error: GARFIELD_INSTALL variable not set.\n";
        return 1;
    }else if (!std::filesystem::exists(fullPath)){
        std::cerr << "Error: File does not exist at " << fullPath << std::endl;
        return 1;
    }
    gas.LoadIonMobility(fullPath);

    // Create a gas volume
    ComponentAnalyticField cmp;
    cmp.SetMedium(&gas);
    
    // Define constants from drift chamber geometry info
    json cellInfo = geomInfo["layers"]["wire_info"];
    json groundInfo = cellInfo["ground_wires"];
    json senseInfo = cellInfo["sense_wires"];

    const double rSense = senseInfo["radius_mm"].get<double>() *0.1;
    const double vSense = senseInfo["voltage_kV"].get<double>() *1000;
    
    const double rGround = groundInfo["radius_mm"].get<double>() *0.1; 
    const double vGround = groundInfo["voltage_kV"].get<double>() *1000;
    
    const double maxL = cellInfo["cell_length_mm"].get<double>() *0.1; 
    const double vTube = 0.;
    const double rTube = 2 * maxL + 0.5; 

    // Add wires to drift cell
    cmp.AddWire(0, 0, 2 * rSense, vSense, "s");
    cmp.AddWire(maxL, maxL, 2 * rGround, vGround, "g1");
    cmp.AddWire(-maxL, maxL, 2 * rGround, vGround, "g2");
    cmp.AddWire(-maxL, -maxL, 2 * rGround, vGround, "g3");
    cmp.AddWire(maxL, -maxL, 2 * rGround, vGround, "g4");
    cmp.AddWire(0, maxL, 2 * rGround, vGround, "g5");
    cmp.AddWire(0, -maxL, 2 * rGround, vGround, "g6");
    cmp.AddWire(-maxL, 0, 2 * rGround, vGround, "g7");
    cmp.AddWire(maxL, 0, 2 * rGround, vGround, "g8");

    // Create a volume to house drift cell
    cmp.AddTube(rTube, vTube, 0);

    // Define the magnetic field in the volume
    json BFieldJson = geomInfo["magnetic_field_teslas"];
    cmp.SetMagneticField(BFieldJson["x"].get<double>(), 
                         BFieldJson["y"].get<double>(), 
                         BFieldJson["z"].get<double>());
                         
    // Add a sensor on the sense wire to detect particles
    Sensor sensor;
    sensor.AddComponent(&cmp);
    sensor.AddElectrode(&cmp, "s");

    const double tstep = 0.5;
    const double tmin = -0.5 * tstep;
    const unsigned int nbins = 8000;
    
    sensor.SetTimeWindow(tmin, tstep, nbins);
    if (!readTransferFunction(sensor)) return 1;
    sensor.ClearSignal();
        
    // Define a particle to be sent through the volume along a track
    TrackHeed track;
    track.SetParticle(geomInfo["particle"]["particle_name"].get<std::string>());
    track.SetEnergy(geomInfo["particle"]["energy_GeV"].get<double>() * 1.e9); 
    track.SetSensor(&sensor);

    // RKF integration
    DriftLineRKF drift(&sensor);
    drift.SetGainFluctuationsPolya(geomInfo["polya"]["shape_param_m"].get<double>(), 
                                   geomInfo["polya"]["mean_gain_G"].get<double>());
    
    // Plot signal
    //TCanvas* cS = new TCanvas("cS", "", 600, 600);

    const double stepSize = 0.1;
    const int totalBins = static_cast<int>(std::ceil(2 * maxL / stepSize));
    
    // Create a maxL x maxL table of drift times at locations in drift cell
    std::vector<std::vector<double>> lookupTable(totalBins, std::vector<double>(totalBins, -1.0));
    std::vector<std::vector<std::pair<double, double>>> diffusionTable(
    totalBins, std::vector<std::pair<double, double>>(totalBins, { -1.0, -1.0 }));


    int rowCount = 0;
    double offset = 0.05; // Offset position from ground wire coord
    
    // Create a maxL x maxL grid of particle tracks
    for (double curY = maxL; curY >= -maxL-offset; curY -= stepSize) {
        sensor.ClearSignal();
        
        double x = -maxL-offset;
        double dx = 2 * (maxL+offset);
        double dy = 0.;
        track.NewTrack(x, curY, 0., 0., dx, dy, 0.);
        
        std::vector<double> times;
        
        // Look at the ionization clusters along the track
        for (const auto& cluster : track.GetClusters()) {
            times.push_back(cluster.t);
            
            for (const auto& electron : cluster.electrons) {
                drift.DriftElectron(electron.x, electron.y, electron.z, electron.t);
                
                double xf = 0., yf = 0., zf = 0., tf = 0.;
                int stat = 0;
                drift.GetEndPoint(xf, yf, zf, tf, stat);
                
                // Add the drift time of electron to the lookup table
                int curi = static_cast<int>(std::floor((electron.x + maxL) / stepSize));
                int curj = static_cast<int>(std::floor((electron.y + maxL) / stepSize));
                
                if (curi >= 0 && curi < totalBins && curj >= 0 && curj < totalBins) {
                    if (lookupTable[curi][curj] == -1.) {
                        lookupTable[curi][curj] = tf - electron.t;
                        
                        xyzVector position = {xf +offset, yf +offset, zf +offset};
                        
                        // Get diffusion constants for this cell
                        std::pair<double, double> diffusion = getDiffusion(&sensor, &cmp, &gas, position);
                        diffusionTable[curi][curj] = diffusion;
                    }
                }
            }
        }
        
        sensor.ConvoluteSignals();
        
        int nt = 0;
        
        /*
        // Write out signal data for a single track
        if (rowCount == 3){
          std::ofstream csvFile;
          csvFile.open("signal_data.csv");
          csvFile << "time_ns,signal\n";
          if (!sensor.ComputeThresholdCrossings(-2., "s", nt)) continue;
              
          sensor.PlotSignal("s", cS);
          
          for (std::size_t i = 0; i < nbins; ++i) {
            const double t = (i + 0.5) * tstep; 
            const double signal = sensor.GetSignal("s", i);
            csvFile << t << "," << signal << "\n";
          }
          csvFile.close();
          
          // Write out actual cluster times
          csvFile.open("actual_cluster_times.txt");
          for (const auto& time : times){
              csvFile << time << "\n";
          }
          csvFile.close();
        }
        */
        
        std::cout << "Row altitude Y = " << curY << " processed (Step #" << ++rowCount << ")\n";

    }
    
    std::ofstream lookupFile("drift_times_lookup.csv");
    std::ofstream diffusionFile("diffusion_lookup.csv");
    
    // Save lookup table data to csv file
    for (int i=0; i<totalBins; i++){
        std::string rowData = "";
        std::string diffRowData = "";
        
        // Column 1 has junk in it so start at j=1
        for (int j=1; j<totalBins-1; j++){
            rowData += std::to_string(lookupTable[i][j]);
            if (j != (totalBins-1)) rowData += ",";
                
            auto [dl, dt]  = diffusionTable[i][j];
            diffRowData += std::to_string(dl) + "|" + std::to_string(dt);
            if (j != (totalBins-1)) diffRowData += ",";
        }
        lookupFile << rowData << "\n";
        diffusionFile << diffRowData << "\n";
    }
    
    lookupFile.close();
    diffusionFile.close();
    
    //app.Run(kTRUE);
    
    return 0;
}

