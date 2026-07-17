/*
Creates a drift chamber with either a single drift cell, a 3x3 grid of drift
cells, or a set of circular drift cell rings.

Writes out wire sag and stretch data to: "sag_wires_data.csv"

Gas file must be changed manually. Run with: "run_sag.sh"
*/

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstdio>

#include <TCanvas.h>
#include <TROOT.h>
#include <TApplication.h>
#include "TMath.h"
#include <cmath>

#include "Garfield/ViewDrift.hh"
#include "Garfield/ComponentAnalyticField.hh"
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/DriftLineRKF.hh"
#include "Garfield/TrackHeed.hh"
#include "Garfield/ViewIsochrons.hh"
#include "Garfield/ViewField.hh"
#include "Garfield/ViewCell.hh"

using namespace Garfield;

auto response = [](double t) {
  const double tau = 8.; 
  if (t < 0) return 0.0;
    
  return (t/tau) * std::exp(-t/tau);
};

double getX(double r, double phi){
  return r * TMath::Cos(phi);
}

double getY(double r, double phi){
  return r * TMath::Sin(phi);
}

bool readTransferFunction(Sensor& sensor) {

  std::ifstream infile;
  infile.open("mdt_elx_delta.txt", std::ios::in);
  if (!infile) {
    std::cerr << "Could not read delta response function.\n";
    return false;
  }
  std::vector<double> times;
  std::vector<double> values;
  while (!infile.eof()) {
    double t = 0., f = 0.;
    infile >> t >> f;
    if (infile.eof() || infile.fail()) break;
    times.push_back(1.e3 * t);
    values.push_back(f);
  }
  infile.close();
  sensor.SetTransferFunction(response);
  return true;
}

int main(int argc, char* argv[]) {
  
  // Check all cl arguments correctly passed
  if (argc < 3) {
    std::cerr << "Error in " << argv[0] << ": Incorrect number of arguments\n";
    return 1;
  }
  
  // 0 = one cell, 1 = 3x3 cell grid, 2 = circular cell grid
  int wireType = std::stoi(argv[1]);
  int isEf = std::stoi(argv[2]);

  TApplication app("app", &argc, argv);
 
  // Make a gas medium.
  MediumMagboltz gas;
  gas.LoadGasFile("he_50_c2h6_50.gas");
  auto installdir = std::getenv("GARFIELD_INSTALL");
  
  if (!installdir) {
    std::cerr << "GARFIELD_INSTALL variable not set.\n";
    return 1;
  }
  
  const std::string path = installdir;
  gas.LoadIonMobility(path + "/share/Garfield/Data/IonMobility_He+_He.txt");

  // Make a component with analytic electric field.
  ComponentAnalyticField cmp;
  cmp.SetMedium(&gas);
  
  // Wire dimensions 
  const double rWire = 25.e-4; //[cm]
  const double lWire = 200.; 
  const double TWire = 50.;    //[g]
  const double TGround = 80.;
  
  // Voltages
  const double vWire = 2730.;
  const double vTube = 0.;
  const double vGround = 0.;
  
  // Outer radius of the tube [cm]
  double rTube;
  
  // ______________SINGLE DRIFT CELL__________________ //
  if (wireType == 0){
    
    // Outer radius of the tube [cm]
    rTube = 0.71;
    
    // Add single drift cell
    cmp.AddWire(0, 0, 2 * rWire, vWire, "s", lWire, TWire);
    cmp.AddWire(-0.3, 0.3, 2 * rWire, vGround, "g1", lWire, TGround);
    cmp.AddWire(0, 0.3, 2 * rWire, vGround, "g2", lWire, TGround);
    cmp.AddWire(0.3, 0.3, 2 * rWire, vGround, "g3", lWire, TGround);
    cmp.AddWire(-0.3, 0, 2 * rWire, vGround, "g4", lWire, TGround);
    cmp.AddWire(0.3, 0, 2 * rWire, vGround, "g5", lWire, TGround);
    cmp.AddWire(-0.3, -0.3, 2 * rWire, vGround, "g6", lWire, TGround);
    cmp.AddWire(0, -0.3, 2 * rWire, vGround, "g7", lWire, TGround);
    cmp.AddWire(0.3, -0.3, 2 * rWire, vGround, "g8", lWire, TGround);
  
  }
  
  // ______________3x3 DRIFT CELL GRID__________________ // 
  else if (wireType == 1){
    int gCount = 1;
    int sCount = 1;
    double xpos;
    double ypos = 0.9;
      
    rTube = 0.91;
      
    // Add 3x3 grid of drift cells
    for (int i = 0; i < 7; i++){    //column
      xpos = 0.9;
      
      for (int j = 0; j < 7; j++){  //row
        if (!(i%2) || !(j%2)){
          cmp.AddWire(xpos, ypos, 2 * rWire, vGround, "g" + std::to_string(gCount), lWire, TGround);
          gCount += 1;
        }else{
          cmp.AddWire(xpos, ypos, 2 * rWire, vWire, "s" + std::to_string(sCount), lWire, TWire);
          sCount += 1;
        }
        xpos -= 0.3;
      }
      ypos -= 0.3;
    }
  }
  
  // ______________CIRCULAR WIRE PATTERN__________________ //
  
  else{
    const double pi = M_PI;
    rTube = 0.71;
    
    double r1 = rTube/4;
    double r2 = rTube/8;
    int ringCount = 0;
    bool isFirst = true;
    
    double theta1;
    double theta2;
    double phi1;
    double phi2;
      
    int countSense = 0;
    int countGround = 0;
    
    std::string sense = "s";
    std::string ground = "g";
     
    // Plot rings of wires
    while (r1 < rTube){
        theta1 = 0;
        theta2 = pi/8;
        
        if (isFirst){
          isFirst = false;
          for (int i=0; i<16; i++){
            countGround += 1;
            cmp.AddWire(getX(rTube/8, theta1), getY(rTube/8, theta1), \
              2 * rWire, vGround, ground + std::to_string(countGround));
            theta1 += pi/8;
          }
          r2 += rTube/4;
          continue;
        }
  
      for (int i=0; i<=8; i++){ 
          if (!(ringCount % 2)){
            phi1 = theta1;
            phi2 = theta2;
          } else{
            phi1 = theta2;
            phi2 = theta1;
          }
          theta1 += pi/4;
          theta2 += pi/4;
          
          // Add sense wire
          countSense += 1;
          cmp.AddWire(getX(r1, phi1), getY(r1, phi1), \
              2 * rWire, vWire, sense + std::to_string(countSense));
          
          // Add ground wires
          countGround += 1;
          cmp.AddWire(getX(r1, phi2), getY(r1, phi2), \
              2 * rWire, vGround, ground + std::to_string(countGround));
          countGround += 1;
          cmp.AddWire(getX(r2, phi1), getY(r2, phi1), \
              2 * rWire, vGround, ground + std::to_string(countGround));
          countGround += 1;
          cmp.AddWire(getX(r2, phi2), getY(r2, phi2), \
              2 * rWire, vGround, ground + std::to_string(countGround));
      }
      ringCount += 1;
      r1 += rTube/4;
      r2 += rTube/4;
    }
  
  }
  
  // Add the tube.
  cmp.AddTube(rTube, vTube, 0);
  
  // Turn on mgnetic field
  // cmp.SetMagneticField(0., 0., 1.5);

  // Make a sensor.
  Sensor sensor;
  sensor.AddComponent(&cmp);
  sensor.AddElectrode(&cmp, "s");
  
  // Set the signal time window.
  const double tstep = 0.5;
  const double tmin = -0.5 * tstep;
  const unsigned int nbins = 1000;
  sensor.SetTimeWindow(tmin, tstep, nbins);
  
  // Set the delta reponse function.
  if (!readTransferFunction(sensor)) return 0;
  sensor.ClearSignal();

  // Set up Heed.
  TrackHeed track;
  track.SetParticle("muon");
  track.SetEnergy(2.e9);
  track.SetSensor(&sensor);
  
  // Open csv file to write out data
  std::ofstream sagFile;
  sagFile.open("sag_wires_data.csv");
  sagFile << "wire_label,csag,xsag,ysag,stretch\n"; // header

  // RKF integration.
  DriftLineRKF drift(&sensor);
  drift.SetGainFluctuationsPolya(10., 20000.); // 10. = 10 ns
 
  TCanvas* cD = nullptr;
  ViewDrift driftView;
  constexpr bool plotDrift = true;
  if (plotDrift) {
    cD = new TCanvas("cD", "Drift", 600, 600);
    driftView.SetCanvas(cD);
    drift.EnablePlotting(&driftView);
    track.EnablePlotting(&driftView);
  }
 
  TCanvas* cS = nullptr;
  constexpr bool plotSignal = true;
  if (plotSignal) cS = new TCanvas("cS", "Signal", 600, 600);

  const double rTrack = 0.25;
  const double x0 = rTrack;
  const double y0 = -sqrt(rTube * rTube - rTrack * rTrack);
  const unsigned int nTracks = 1;
  for (unsigned int j = 0; j < nTracks; ++j) {
    sensor.ClearSignal();
    track.NewTrack(x0, y0, 0, 0, 0, 1, 0);
    
    for (const auto& cluster : track.GetClusters()) {
      for (const auto& electron : cluster.electrons) {
        drift.DriftElectron(electron.x, electron.y, electron.z, electron.t);      
      }
    }
    
    if (plotDrift) {
      cD->Clear();
      cmp.PlotCell(cD);
      constexpr bool twod = true;
      constexpr bool drawaxis = false;
      driftView.Plot(twod, drawaxis);
    }
    
    sensor.ConvoluteSignals();
    int nt = 0;
    if (!sensor.ComputeThresholdCrossings(-2., "s", nt)) continue;
    if (plotSignal) sensor.PlotSignal("s", cS);
  } 
  
  // Print number of drift lines
  const std::size_t nDriftLines = driftView.GetNumberOfDriftLines();
  std::cout << "Number of drift lines: " << nDriftLines << std::endl;
  
  // Print info about how much each wire sags
  std::vector<double> csag; 
  std::vector<double> xsag;
  std::vector<double> ysag;
  
  double stretch = 0.;
  std::string csagStr;
  std::string xsagStr;
  std::string ysagStr;
    
  cmp.SetGravity(0., -1., 0.);
  cmp.EnableGravity(true);
  
  if (isEf){
    cmp.EnableElectrostaticForce(true);
  }else{
    cmp.EnableElectrostaticForce(false);
  }
  
  for (int i=0; i<cmp.GetNumberOfWires(); i++){
    csagStr = "";
    xsagStr = "";
    ysagStr = "";
    
    double x, y, diameter, voltage, length, charge;
    std::string label;
    int ntrap;

    cmp.GetWire(i, x, y, diameter, voltage, label, length, charge, ntrap);
    sagFile << label << ",";
    cmp.WireDisplacement(i, false, csag, xsag, ysag, stretch, false);
    
    for (int j = 0; j < csag.size(); ++j) {
       csagStr += std::to_string(csag[j]) + "|";
       xsagStr += std::to_string(xsag[j]) + "|";
       ysagStr += std::to_string(ysag[j]) + "|";
    }
    
    sagFile << csagStr << "," << xsagStr << "," << ysagStr;
    sagFile << "," << stretch << "\n";
  }
  
  // Close csv file
  sagFile.close();

  //app.Run(kTRUE);

}