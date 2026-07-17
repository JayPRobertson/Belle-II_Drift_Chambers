/*
Creates a drift chamber with wires generated in a circular pattern around the origin
and plots the resulting drift lines for one vertical track.

Gas file must be changed manually.
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

#define _USE_MATH_DEFINES

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

int main(int argc, char * argv[]) {

  TApplication app("app", &argc, argv);
 
  // Make a gas medium.
  MediumMagboltz gas;
  gas.LoadGasFile("ar_93_co2_7_3bar.gas");
  auto installdir = std::getenv("GARFIELD_INSTALL");
  
  if (!installdir) {
    std::cerr << "GARFIELD_INSTALL variable not set.\n";
    return 1;
  }
  
  const std::string path = installdir;
  gas.LoadIonMobility(path + "/share/Garfield/Data/IonMobility_Ar+_Ar.txt");

  // Make a component with analytic electric field.
  ComponentAnalyticField cmp;
  cmp.SetMedium(&gas);
  
  // Wire radius [cm]
  const double rWire = 25.e-4;
  
  // Outer radius of the tube [cm]
  const double rTube = 0.71;
  
  // Voltages
  const double vWire = 2730.;
  const double vTube = 0.;
  const double vGround = 0.;
  
  // Add the tube.
  cmp.AddTube(rTube, vTube, 0);
  
  // Constants
  const double pi = M_PI;
  
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
   
  // Plot rings of wires.
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
  
  // Turn on mgnetic field
  // cmp.SetMagneticField(0., 0., 1.5);

  // Make a sensor.
  Sensor sensor;
  sensor.AddComponent(&cmp);
  
  for (int i=1; i<countSense+1; i++){
    sensor.AddElectrode(&cmp, sense + std::to_string(i));
  }
  
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
  track.SetEnergy(170.e9);
  track.SetSensor(&sensor);

  // RKF integration.
  DriftLineRKF drift(&sensor);
  drift.SetGainFluctuationsPolya(10., 20000.); // 10. = 10 ns
  
  // Set up plotting of particles.
  TCanvas* cD = nullptr;
  ViewDrift driftView;
  constexpr bool plotDrift = true;
  if (plotDrift) {
    cD = new TCanvas("cD", "Drift", 600, 600);
    driftView.SetCanvas(cD);
    drift.EnablePlotting(&driftView);
    track.EnablePlotting(&driftView);
  }

  // Set up plotting of signal.
  TCanvas* cS = nullptr;
  constexpr bool plotSignal = true;
  if (plotSignal) cS = new TCanvas("cS", "Signal", 600, 600);

  // Make straight vertical track.
  const double rTrack = 0.1;
  const double x0 = rTrack;
  const double y0 = -sqrt(rTube * rTube - rTrack * rTrack);
  
  track.NewTrack(x0, y0, 0, 0, 0, 1, 0);
  
  // Get drift lines from clusters.
  for (const auto& cluster : track.GetClusters()) {
    for (const auto& electron : cluster.electrons) {
      drift.DriftElectron(electron.x, electron.y, electron.z, electron.t);
    }
  }
  
  // Plot track, drift lines, wires, and tube.
  if (plotDrift) {
    cD->Clear();
    cmp.PlotCell(cD);
    constexpr bool twod = true;
    constexpr bool drawaxis = false;
    driftView.Plot(twod, drawaxis);
  }
  
  // Print number of drift lines
  const std::size_t nDriftLines = driftView.GetNumberOfDriftLines();
  std::cout << "Number of drift lines: " << nDriftLines << std::endl;

  app.Run(kTRUE);

}