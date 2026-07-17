/*
Creates a drift chamber with a single drift cell.

Creates 1000 randomly generated tracks through the drift tube and writes out the
track number, track start position and angle, electron drift time of each track, drift line
starting position along the track, and number of driftlines for each to:
"multitrack_data_file.csv"

Gas file must be changed by CL arg. Run with: "run_randtrack.sh"
*/

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstdio>
#include <numeric>

#include <TCanvas.h>
#include <TROOT.h>
#include <TApplication.h>
#include <TRandom3.h>
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

#include <TH1F.h>
#include <TCanvas.h>

#define _USE_MATH_DEFINES

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
  
  // Constants
  const double pi = M_PI;
  
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
  std::ofstream trackFile;
  trackFile.open("multitrack_data_file.csv");
  trackFile << "track_num,theta,start_x,start_y,drift_times,line_start_x,line_start_y,num_driftlines\n";
  
  std::ofstream otherFile;
  otherFile.open("dNdx_file.csv");
  otherFile << "dNdx,percent_deltas\n";
 
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

  // Get random track position and angle
  TRandom3 rnd(0);
  const double r = rTube-0.01;
  
  double phi1=0, phi2=0., theta=0.;
  double x=0., y=0., dx=0., dy=0.;
    
  const int deltaClusterSizeThreshold = 300; 
  
  std::vector<double> dNdx;
  
  const unsigned int nTracks = 1000;
  for (unsigned int j = 0; j < nTracks; ++j) {
    driftView.Clear();
    
    phi1 = rnd.Uniform(0, 2*pi);
    phi2 = rnd.Uniform(0, 2*pi);
    x = r * TMath::Cos(phi1);
    y = r * TMath::Sin(phi1);
    
    double x2 = r * TMath::Cos(phi2);
    double y2 = r * TMath::Sin(phi2);
    
    dx = x2 - x;
    dy = y2 - y;
    double trackLength = std::sqrt(dx*dx + dy*dy);
    dx /= trackLength;
    dy /= trackLength;
    
    theta = std::acos(dx);

    sensor.ClearSignal();
    
    track.NewTrack(x, y, 0., 0., dx, dy, 0.); 
    
    std::string driftTimesStr = "";
    std::string dlineStartX = "";
    std::string dlineStartY = "";
    
    double xc = 0., yc = 0., zc = 0., tc = 0., ec = 0., extra = 0.;
    int ne = 0;
    
    unsigned int clusterCount = 0;
    unsigned int deltaCount = 0;

    while (track.GetCluster(xc, yc, zc, tc, ne, ec, extra)) {
      clusterCount++;

      if (ne > deltaClusterSizeThreshold) {
        deltaCount++;
      }
    
      for (int i = 0; i < ne; ++i) {
        double xe = 0., ye = 0., ze = 0., te = 0., ee = 0., dxs = 0., dys = 0., dzs = 0.;
        track.GetElectron(i, xe, ye, ze, te, ee, dxs, dys, dzs);
        
        drift.DriftElectron(xe, ye, ze, te);
        points.push_back({xe, ye, ze});
        
        double xf = 0., yf = 0., zf = 0., tf = 0.;
        int stat = 0;
        drift.GetEndPoint(xf, yf, zf, tf, stat);
        double driftTime = tf - te;
        
        dlineStartX += std::to_string(xe) + "|";
        dlineStartY += std::to_string(ye) + "|";
        driftTimesStr += std::to_string(driftTime) + "|";
      }
    }

    double experimental_dNdx = static_cast<double>(clusterCount) / trackLength;
    dNdx.push_back(experimental_dNdx);
    
    if (deltaCount){
       std::cout << deltaCount << "     " << clusterCount << std::endl;
    }

    double percentDeltas = (clusterCount > 0) ? (static_cast<double>(deltaCount) / clusterCount) * 100.0 : 0.0;
    otherFile << experimental_dNdx << "," << percentDeltas << "\n";
    
    const std::size_t nDriftLines = driftView.GetNumberOfDriftLines();

    trackFile << j << "," << theta << "," << x << "," << y << "," << driftTimesStr;
    trackFile << "," << dlineStartX <<  "," << dlineStartY << "," << nDriftLines << "\n"; 
    
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
    trackFile.close(); 
    
    otherFile.close();
    
    double minVal = *std::min_element(dNdx.begin(), dNdx.end());
    double maxVal = *std::max_element(dNdx.begin(), dNdx.end());
      
    TH1F *hist = new TH1F("hist", "Dataset Distribution 1000 tracks;dN/dx;Frequency", 25, minVal-10., maxVal+10.);
    
    for (double value: dNdx) { hist->Fill(value); }
      
    TCanvas *canvas = new TCanvas("canvas", "dN/dx Frequency", 800, 600);
    hist->Draw();
    canvas->SaveAs("dNdx_distribution.png");
    
    double sum_dNdx = std::accumulate(dNdx.begin(), dNdx.end(), 0.0);
    double mean_dNdx = sum_dNdx / dNdx.size();
    std::cout << "Mean dN/dx: " << mean_dNdx << " clusters/cm" << std::endl;

  //app.Run(kTRUE);

}