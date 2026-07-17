/*
Creates a drift chamber with a single drift cell and plots the resulting drift
lines, isochrons, signal, and electric field for one vertical track.

Writes out electric field strength on the track, drift time, drift distance of each 
drift line to: "drift_data_file.csv"

Gas file must be changed by CL arg. Run with: "run_mdt.sh"
*/

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstdio>

#include <TCanvas.h>
#include <TROOT.h>
#include <TApplication.h>

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
  if (argc < 4) {
    std::cerr << "Error in " << argv[0] << ": Incorrect number of arguments\n";
    return 1;
  }
  
  std::string gasFilename = argv[1];
  std::string ionMobilityFilename = argv[2];
  int isBField = std::stoi(argv[3]);
    
  argc = 1;
  TApplication app("app", &argc, argv);
 
  // Make a gas medium.
  MediumMagboltz gas;
  gas.LoadGasFile(gasFilename);
  auto installdir = std::getenv("GARFIELD_INSTALL");
  
  if (!installdir) {
    std::cerr << "GARFIELD_INSTALL variable not set.\n";
    return 1;
  }
  
  const std::string path = installdir;
  gas.LoadIonMobility(path + "/share/Garfield/Data/" + ionMobilityFilename);

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
  
  // Add drift cell
  cmp.AddWire(0, 0, 2 * rWire, vWire, "s");
  
  cmp.AddWire(0.3, 0.3, 2 * rWire, vGround, "g1");
  cmp.AddWire(-0.3, 0.3, 2 * rWire, vGround, "g2");
  cmp.AddWire(-0.3, -0.3, 2 * rWire, vGround, "g3");
  cmp.AddWire(0.3, -0.3, 2 * rWire, vGround, "g4");
  
  cmp.AddWire(0, 0.3, 2 * rWire, vGround, "g5");
  cmp.AddWire(0, -0.3, 2 * rWire, vGround, "g6");
  cmp.AddWire(-0.3, 0, 2 * rWire, vGround, "g7");
  cmp.AddWire(0.3, 0, 2 * rWire, vGround, "g8");
  
  // Add the tube.
  cmp.AddTube(rTube, vTube, 0);
  
  // Turn on mgnetic field
  if (isBField) {
    cmp.SetMagneticField(0., 0., 1.5);
  }

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

  // RKF integration.
  DriftLineRKF drift(&sensor);
  drift.SetGainFluctuationsPolya(10., 20000.); // 10. = 10 ns
  
  std::vector<std::array<double, 3>> points;
  
  // Open csv file to write out data
  std::ofstream driftFile;
  driftFile.open("drift_data_file.csv");
  driftFile << "Distance,Time,EField\n"; // header
 
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
        points.push_back({electron.x, electron.y, electron.z});
        
        // Get drift time
        double xf = 0., yf = 0., zf = 0., tf = 0.;
        int stat = 0;
        drift.GetEndPoint(xf, yf, zf, tf, stat);
        
        // Get electric field at starting point on track
        double ex = 0., ey = 0., ez = 0., v = 0.;
        Medium* m = nullptr;
        sensor.ElectricField(electron.x, electron.y, electron.z, ex, ey, ez, v, m, stat);
        double eMag = std::sqrt(ex*ex + ey*ey + ez *ez);
        
        // Get theoretical drift distance
        double xDistance = electron.x - xf;
        double yDistance = electron.y - yf;
        double zDistance = electron.z - zf;
        double driftTime = tf - electron.t;
        double driftDist = std::sqrt(xDistance*xDistance + yDistance*yDistance + zDistance*zDistance);
        
        // Write out drift data as csv
        driftFile << driftDist << "," << driftTime << "," << eMag << "\n";    
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
  
  // Close csv file
  driftFile.close();
  
  // Print number of drift lines
  const std::size_t nDriftLines = driftView.GetNumberOfDriftLines();
  std::cout << "Number of drift lines: " << nDriftLines << std::endl;

  //Plotting disabled to run with .sh file
  //app.Run(kTRUE);

}