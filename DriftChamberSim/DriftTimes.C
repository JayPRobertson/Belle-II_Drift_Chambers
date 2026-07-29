#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstdio>
#include <vector>
#include <array>
#include <cmath>

#include <TCanvas.h>
#include <TROOT.h>
#include <TApplication.h>
#include <TRandom3.h>
#include "TMath.h"

#include "Garfield/ViewDrift.hh"
#include "Garfield/ComponentAnalyticField.hh"
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/DriftLineRKF.hh"
#include "Garfield/TrackHeed.hh"

#define _USE_MATH_DEFINES

using namespace Garfield;

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

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Error in " << argv[0] << ": Incorrect number of arguments\n";
        return 1;
    }
    std::string gasFilename = argv[1];
    std::string ionMobilityFilename = argv[2];
    int isBField = std::stoi(argv[3]);

    int app_argc = 1;
    TApplication app("app", &app_argc, argv);

    MediumMagboltz gas;
    gas.LoadGasFile(gasFilename);
    
    auto installdir = std::getenv("GARFIELD_INSTALL");
    if (!installdir) {
        std::cerr << "GARFIELD_INSTALL variable not set.\n";
        return 1;
    }
    const std::string path = installdir;
    gas.LoadIonMobility(path + "/share/Garfield/Data/" + ionMobilityFilename);

    ComponentAnalyticField cmp;
    cmp.SetMedium(&gas);

    const double rWire = 0.0126; 
    const double rTube = 3.0; 
    const double vWire = 2730.;
    const double vTube = 0.;
    const double vGround = 0.;
    const double maxL = 1.8; 

    cmp.AddWire(0, 0, 2 * rWire, vWire, "s");
    cmp.AddWire(maxL, maxL, 2 * rWire, vGround, "g1");
    cmp.AddWire(-maxL, maxL, 2 * rWire, vGround, "g2");
    cmp.AddWire(-maxL, -maxL, 2 * rWire, vGround, "g3");
    cmp.AddWire(maxL, -maxL, 2 * rWire, vGround, "g4");
    cmp.AddWire(0, maxL, 2 * rWire, vGround, "g5");
    cmp.AddWire(0, -maxL, 2 * rWire, vGround, "g6");
    cmp.AddWire(-maxL, 0, 2 * rWire, vGround, "g7");
    cmp.AddWire(maxL, 0, 2 * rWire, vGround, "g8");

    cmp.AddTube(rTube, vTube, 0);

    if (isBField) {
        cmp.SetMagneticField(0., 0., 1.5);
    }

    Sensor sensor;
    sensor.AddComponent(&cmp);
    sensor.AddElectrode(&cmp, "s");

    const double tstep = 0.5;
    const double tmin = -0.5 * tstep;
    const unsigned int nbins = 1000;
    sensor.SetTimeWindow(tmin, tstep, nbins);

    if (!readTransferFunction(sensor)) return 1;

    TrackHeed track;
    track.SetParticle("muon");
    track.SetEnergy(2.e9); 
    track.SetSensor(&sensor);

    DriftLineRKF drift(&sensor);
    drift.SetGainFluctuationsPolya(10., 20000.);

    ViewDrift driftView;
    TCanvas* cD = new TCanvas("cD", "Drift", 600, 600);
    driftView.SetCanvas(cD);
    drift.EnablePlotting(&driftView);
    track.EnablePlotting(&driftView);

    TCanvas* cS = new TCanvas("cS", "Signal", 600, 600);

    const double stepSize = 0.1;
    const int totalBins = static_cast<int>(std::ceil(2 * maxL / stepSize));
    std::vector<std::vector<double>> lookupTable(totalBins, std::vector<double>(totalBins, -1.0));

    int rowCount = 0;

    for (double curY = maxL; curY >= -maxL-0.05; curY -= stepSize) {
        driftView.Clear();
        sensor.ClearSignal();
        
        double x = -maxL-0.05;
        double dx = 2 * (maxL+0.05);
        double dy = 0.;
        track.NewTrack(x, curY, 0., 0., dx, dy, 0.);

        for (const auto& cluster : track.GetClusters()) {
            for (const auto& electron : cluster.electrons) {
                drift.DriftElectron(electron.x, electron.y, electron.z, electron.t);
                
                double xf = 0., yf = 0., zf = 0., tf = 0.;
                int stat = 0;
                drift.GetEndPoint(xf, yf, zf, tf, stat);
                
                int curi = static_cast<int>(std::floor((electron.x + maxL) / stepSize));
                int curj = static_cast<int>(std::floor((electron.y + maxL) / stepSize));
                
                if (curi >= 0 && curi < totalBins && curj >= 0 && curj < totalBins) {
                    if (lookupTable[curi][curj] == -1.) {
                        lookupTable[curi][curj] = tf - electron.t; 
                    }
                }
            }
        }

        std::cout << "Row altitude Y = " << curY << " processed (Step #" << ++rowCount << ")\n";

        cD->Clear();
        cmp.PlotCell(cD);
        driftView.Plot(true, false);

        //sensor.ConvoluteSignals();
        //int nt = 0;
        //if (sensor.ComputeThresholdCrossings(-2., "s", nt)) {
            //sensor.PlotSignal("s", cS);
        //}
    }
    
    std::ofstream lookupFile;
    lookupFile.open("drift_times_lookup.csv");
    
    // Print lookup table data
    for (int i=0; i<totalBins; i++){
        std::string rowData = "";
        for (int j=0; j<totalBins; j++){
            rowData += std::to_string(lookupTable[i][j]);
            if (j != (totalBins-1)) rowData += ",";
        }
        lookupFile << rowData << "\n";
    }
    
    lookupFile.close();
    
    
    // app.Run(kTRUE); 
    return 0;
}

