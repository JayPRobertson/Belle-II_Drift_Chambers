#include <iostream>
#include <fstream>
#include <sstream>

#include <TFile.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>

#include <TVector3.h>
#include <TVectorD.h>
#include <TMatrixDSym.h>

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <TRandom3.h>

#include <WireMeasurementNew.h>
#include <Track.h>
#include <RKTrackRep.h>
#include <KalmanFitterRefTrack.h>
#include <MaterialEffects.h>
#include <FieldManager.h>
#include <ConstField.h>

#include "G4ThreeVector.hh"
#include "TVector3.h"

#include "KalmanFit.hh"
#include "RandomGenerator.hh"

using CellHit = DriftChamberSim::KalmanFit::CellHit;
using ParticleConstants = DriftChamberSim::TrackedParticle::ParticleConstants;

namespace DriftChamberSim {
    
const double TWOPI = 2. * M_PI;

void KalmanFit::ProcessParticleTracks(){
    
    // Open ROOT files
    TFile* file = TFile::Open("particle_and_track_data.root");     // in
    TFile* outFile = new TFile("kalman_output.root", "RECREATE");  // out
    
    if (!file || file->IsZombie()) {
        std::cerr << "Error opening file particle_and_track_data.root" << std::endl;
        return;
    } else if (!outFile || outFile->IsZombie()) {
        std::cerr << "Error opening file kalman_output.root" << std::endl;
        if (file) file->Close();
        return;
    }
    
    TTree* outTree = new TTree("Particles", "Particles");
    
    // Set up ROOT tree branches
    std::vector<LayerHit> actualHits;
    outTree->Branch("KalmanFitted", &kalmanHits); 
    outTree->Branch("Actual", &actualHits);
    
    // Get number of wires in each layer of detector
    numWiresPerLayer = GetWiresPerLayer();
    if (!numWiresPerLayer.size()) {
        file->Close();
        outFile->Close();
        delete file;
        delete outFile;
        std::cout << "Error: No wires in numWiresPerLayer" << std::endl;
        return;
    }
    
    TTree* rootTree = (TTree*)file->Get("Particles");
    TTreeReader reader(rootTree);
    
    std::map<int, std::vector<LayerHit>> trackMap;
    std::map<int, xyzVector> momMap;
    
    TTreeReaderValue<ParticleConstants> constants(reader, "Constants");
    
    while (reader.Next()) {
        BField = {constants->getBField().X(), constants->getBField().Y(), constants->getBField().Z()};
        particleMass = constants->getMass();
        particleCharge = constants->getCharge();
        break;
    }
    
    reader.Restart();
    TTreeReaderValue<std::vector<TrackedParticle>> particles(reader, "Particle");            
    
    // Process input data
    while (reader.Next()) {
        for (const auto& particle : *particles) {
          int trackID = particle.getID();
              
          xyzVector initMom = {particle.getMomentum().X(), 
                               particle.getMomentum().Y(), 
                               particle.getMomentum().Z()};
          momMap[trackID] = initMom;
   
          for (const auto& seg : particle.getTrackSegments()) {
              LayerHit hit;
              hit.entryPos = {seg.getEntryPosition().X(), 
                              seg.getEntryPosition().Y(), 
                              seg.getEntryPosition().Z()};
              hit.entryMom = { seg.getEntryMomentum().X(), 
                              seg.getEntryMomentum().Y(), 
                              seg.getEntryMomentum().Z()};
              hit.exitPos = { seg.getExitPosition().X(), 
                              seg.getExitPosition().Y(), 
                              seg.getExitPosition().Z()};
              hit.exitMom = { seg.getExitMomentum().X(), 
                              seg.getExitMomentum().Y(), 
                              seg.getExitMomentum().Z()};
                                  
              hit.entryTime = seg.getEntryTime();
              hit.exitTime = seg.getExitTime();
              hit.edep = seg.getEnergyLoss();
              hit.layerID = seg.getLayerIndex();
              hit.trackID = trackID;
              hit.initMomMag = std::sqrt(initMom.X()*initMom.X() + initMom.Y()*initMom.Y());
              
              trackMap[trackID].push_back(hit);
              actualHits.push_back(hit);
          }
        }
    }
    
    // Sort actual hits by trackID
    std::sort(actualHits.begin(), actualHits.end(), 
           [](const LayerHit& a, const LayerHit& b) {
                return a.trackID < b.trackID;
    });
    
    // Initialize GenFit
    std::cout << "Initializing GenFit field" << std::endl;
    genfit::FieldManager::getInstance()->init(new genfit::ConstField(BField.X() *1.e4,
                                                                     BField.Y() *1.e4,
                                                                     BField.Z() *1.e4)); 
    genfit::MaterialEffects::getInstance()->setNoEffects();
    
    IndexMap timeTable = GetLookupTable("drift_times_lookup.csv");
    IndexMap diffusionTable = GetLookupTable("diffusion_lookup.csv");
    
    int count = 0;
    int arbitraryTrackNum = 2;
    
    // Perform fits and write out complete rows
    for (auto& pair : trackMap) {
        int i = pair.first;
        auto& hits = pair.second;

        std::sort(hits.begin(), hits.end(), 
           [](const LayerHit& a, const LayerHit& b) {
                return a.layerID < b.layerID;
        });

        std::vector<CellHit> detectedCells = GetDetectedWires(hits);
        
        if (count == arbitraryTrackNum){
            GetClusterInfo(detectedCells, timeTable, diffusionTable, momMap);
        }
        count++;
        
        GetKalmanFit(detectedCells, i, momMap[i]);
    }

    outTree->Fill();
    outTree->Write();
    
    // Free up memory
    outFile->Close();
    file->Close();

    delete file;
    delete outFile;
}

std::vector<CellHit> KalmanFit::GetDetectedWires(const std::vector<LayerHit>& sortedHits){
    std::vector<CellHit> detectedCells;

    for (const auto& layerHit : sortedHits) {
        int layerIndex = layerHit.layerID - 20000;

        auto it = numWiresPerLayer.find(layerIndex);
        if (it == numWiresPerLayer.end()) {
            std::cerr << "Warning: no wire count for layer "
                      << layerIndex << std::endl;
            continue;
        }

        int n = std::get<0>(it->second);
        double wireLength = std::get<1>(it->second);
        double delta = TWOPI / static_cast<double>(n);

        for (int i = 0; i < n; ++i) {
            HitData wireInfo = GetClosestWire(layerHit);
            
            // Only take the first hit in a layer for now
            xyzVector wirePosition = wireInfo[0].first;
            std::pair<xyzVector, xyzVector> wireEnds = wireInfo[0].second;
            
            double exitTime = layerHit.exitTime;
            WireInfo wireData = {wireLength, wireEnds.first, wireEnds.second};
            
            detectedCells.push_back({
                wirePosition,
                layerHit.entryMom,
                layerIndex,
                i,
                layerHit.entryTime,
                exitTime,
                layerHit.entryPos,
                layerHit.exitPos,
                wireData
            });

            break;
        }
    }

    return detectedCells;
}

void KalmanFit::GetClusterInfo(std::vector<CellHit> detectedCells, IndexMap timeTable, IndexMap diffusionTable, std::map<int, xyzVector> momMap){
    auto ranInstance = RandomGenerator::instance();
    
    std::ofstream clusterFile("cluster_info.csv");
    clusterFile << "gen_time,drift_time,long_diffusion,trans_diffusion\n";
    
    // Look at the track segment through each cell it hits
    for (auto cell : detectedCells){
        int layerIndex = cell.layerID;
        int wireID = cell.cellID;
        int totalWires = std::get<0>(numWiresPerLayer[layerIndex]);
        double deltaPhi = TWOPI / static_cast<double>(totalWires);

        // Wire endpoints
        xyzVector wEnd1 = cell.wireInfo.end1;
        xyzVector wEnd2 = cell.wireInfo.end2;
        
        double phiW1 = std::atan2(wEnd1.Y(), wEnd1.X());
        double phiW2 = std::atan2(wEnd2.Y(), wEnd2.X());
        if (phiW2 - phiW1 > M_PI)  phiW1 += TWOPI;
        if (phiW1 - phiW2 > M_PI)  phiW2 += TWOPI;

        double dPhidZ = (phiW2 - phiW1) / (wEnd2.Z() - wEnd1.Z());

        // Function to check if point in this cell
        auto IsInsideCell = [&](const G4ThreeVector& pos) {
            double pPhi = std::atan2(pos.y(), pos.x());
            double expectedWirePhi = phiW1 + dPhidZ * (pos.z() - wEnd1.Z());
            
            // Normalize angular difference to (-PI, PI]
            double dPhi = pPhi - expectedWirePhi;
            while (dPhi > M_PI)  dPhi -= TWOPI;
            while (dPhi <= -M_PI) dPhi += TWOPI;

            // Cell boundaries are halfway to the next wires
            return std::abs(dPhi) <= (deltaPhi / 2.0);
        };

        HelixApproach helix = GetHelix(cell);
        double totalTime = cell.t2 - cell.t1;
        
        double cellT1 = cell.t1;
        double cellT2 = cell.t2;
        int steps = 100;
        
        bool foundEntry = false;
        for (int step = 0; step <= steps; ++step) {
            double frac = static_cast<double>(step) / steps;
            double ut = frac * totalTime;
            G4ThreeVector pos = helix.Position(ut);
            
            if (IsInsideCell(pos)) {
                if (!foundEntry) {
                    cellT1 = cell.t1 + ut; // First time inside cell
                    foundEntry = true;
                }
                cellT2 = cell.t1 + ut;     // Last time inside cell
            }
        }

        // Calculate the actual path length inside the cell
        G4ThreeVector cellEntryPos = helix.Position(cellT1 - cell.t1);
        G4ThreeVector cellExitPos = helix.Position(cellT2 - cell.t1);
        
        double cellLength = std::sqrt(std::pow(cellExitPos.x() - cellEntryPos.x(), 2) +
                                      std::pow(cellExitPos.y() - cellEntryPos.y(), 2) +
                                      std::pow(cellExitPos.z() - cellEntryPos.z(), 2));

        int numClusters = ranInstance.fromPoisson(cellLength * avgNumClusters);
        
        // Randomly generate clusters along the track in the cur cell
        for (int i=0; i < numClusters; i++){
            
            // Find position along helix at time ut
            double ut = ranInstance.fromUniform(cellT1 - cell.t1, cellT2 - cell.t1); 
            G4ThreeVector clusterOrigin = helix.Position(ut);
            
            int curi = static_cast<int>(std::round(clusterOrigin.x()));
            int curj = static_cast<int>(std::round(clusterOrigin.y()));
            
            // Make positive index starting at 0
            curi *= (curi > 0) ? 2 : -1;
            curj *= (curj > 0) ? 2 : -1;
            
            std::string diffusion;
           
            // Get the two diffusion values stored in the lookup table string
            try {
                diffusion = diffusionTable[curi][curj];
                if (diffusion.empty()) continue;
            } catch(const std::exception& e){
                i--;
                continue;
            }

            std::stringstream ss(diffusion);
            std::string token;
            std::vector<std::string> diffusions;

            while (std::getline(ss, token, '|')) {
                diffusions.push_back(token);
            }
            
            if (diffusions.size() < 2) continue;
                
            // Get info about the cur cluster
            double driftTime = std::stod(timeTable[curi][curj]);
            double longDiffusion = std::stod(diffusions[0]);
            double transDiffusion = std::stod(diffusions[1]);
                      
            clusterFile << ut << "," << driftTime << "," << longDiffusion << ","
                        << transDiffusion << "\n";
        }
    }
    
    clusterFile.close();
}

// Gets the Kalman fit of a track based on its layer hits
void KalmanFit::GetKalmanFit(std::vector<CellHit> detectedCells, int trackID, xyzVector initMomentum){
    
    // Sort measurements from inner radius to outer radius.
    std::sort(detectedCells.begin(), detectedCells.end(),
        [](const CellHit& a, const CellHit& b) {
            return a.wirePos.perp2() < b.wirePos.perp2();
    });

    if(detectedCells.size() > 300 || detectedCells.size() < 5){
        std::cout << "Skipping track " << trackID 
                  << " too many or too few hits. Number of hits = " << detectedCells.size() 
                  << std::endl;
    
        return;
    }

    const xyzVector& p1 = detectedCells.front().wirePos;

    TVector3 initPos(p1.X()/10., p1.Y()/10., p1.Z()/10.); //[cm]
    size_t directionIndex = std::min(static_cast<size_t>(5), detectedCells.size()-1);

    const xyzVector& p2 = detectedCells[directionIndex].wirePos;
    xyzVector direction = (p2-p1).unit();

    double momentum = initMomentum.R() / 1000.; //[GeV]

    TVector3 initMom(
        direction.X() * momentum,
        direction.Y() * momentum,
        direction.Z() * momentum
    );

    // Create genfit track
    auto* rep = new genfit::RKTrackRep(13);
    auto* gfTrack = new genfit::Track(rep, initPos, initMom);
    
    // Set covariance seed
    TMatrixDSym covSeed(6); 
    covSeed(0,0) = 1; 
    covSeed(1,1) = 1; 
    covSeed(2,2) = 2*2; 
    covSeed(3,3) = 0.1*0.1; 
    covSeed(4,4) = 0.1*0.1; 
    covSeed(5,5) = 0.2*0.2; 
    gfTrack->setCovSeed( covSeed );

    int hitID = 0;
    const double wireHalfLength = 1000.; // Arbitrary length

    for (const auto& cell : detectedCells) {
        double tmid = 0.75 * (cell.t1 + cell.t2);
        
         // Find shortest drift distance between track in the cell and wire
        HelixApproach helix = GetHelix(cell);
        double increment = (tmid - cell.t1)/1000.;
        double driftDistance = cell.wireInfo.wireLength /10.; 
        bool isIncreasing = false;
        
        for (double t = 0.; t < (tmid - cell.t1); t+=increment){
            G4ThreeVector pos = helix.Position(t);
            
            // Convert positions to [cm]
            double curDistance = std::hypot(pos.x()/10. - cell.wirePos.X() / 10., 
                                            pos.y()/10. - cell.wirePos.Y() / 10.,
                                            pos.z()/10. - cell.wirePos.Z() / 10.);
                
            if (curDistance < driftDistance){
                driftDistance = curDistance;
            }else{
                isIncreasing = true;
            }
            if (isIncreasing) break;
        }
        
        const double driftDistanceError = 1.0 * driftDistance;
        
        xyzVector end1 = cell.wireInfo.end1;
        xyzVector end2 = cell.wireInfo.end2;
        
        TVector3 endPoint1(end1.X() / 10., end1.Y() / 10., end1.Z() / 10.);
        TVector3 endPoint2(end2.X() / 10., end2.Y() / 10., end2.Z() / 10.);

        auto* measurement = new genfit::WireMeasurementNew(
                driftDistance,
                driftDistanceError,
                endPoint1,
                endPoint2,
                0,           // detID
                hitID,       // hitID         
                nullptr      // trackPoint
        );

        gfTrack->insertMeasurement(measurement);

        ++hitID;
    }

    genfit::KalmanFitterRefTrack fitter;

    fitter.setMaxIterations(100);
    fitter.setMultipleMeasurementHandling(
        genfit::EMultipleMeasurementHandling::unweightedClosestToReferenceWire
    );

    try {
        fitter.processTrack(gfTrack);
        auto* status = gfTrack->getFitStatus(rep);

        std::cout << ">>> Track = " << trackID
                  << ", hits = " << detectedCells.size()
                  << ", fitted = " << status->isFitted()
                  << ", converged = " << status->isFitConverged()
                  << ", chi2 = " << status->getChi2()
                  << ", ndf = " << status->getNdf() 
                  << ", X2v = " << status->getChi2()/status->getNdf() << std::endl;

        if (status->isFitConverged()) {
            genfit::MeasuredStateOnPlane state = gfTrack->getFittedState();

            double s = 100.;
            bool passedOrigin = false;
            const double epsilon = 5.;

            while (!passedOrigin && std::abs(s) < 200.) {
                genfit::MeasuredStateOnPlane sampleState(state);

                try {
                    rep->extrapolateBy(sampleState, s);
                    TVector3 pos, mom;
                    sampleState.getPosMom(pos, mom);

                    // Check if cur pos is ~origin
                    if (std::abs(pos.X()) < epsilon && 
                        std::abs(pos.Y()) < epsilon && 
                        std::abs(pos.Z()) < epsilon) {
                        passedOrigin = true;
                    }

                    KalmanHit kHit;
                    
                    // Convert data to Geant4 units ([mm] and [MeV])
                    kHit.hitPos.SetXYZ(pos.X() *10., pos.Y() *10., pos.Z() *10.);
                    kHit.hitMom.SetXYZ(mom.X() *1000., mom.Y() *1000., mom.Z() *1000.);

                    kHit.chi2 = status->getChi2();
                    kHit.ndf = status->getNdf();
                    kHit.trackID = trackID;

                    kalmanHits.push_back(kHit);
                    
                } catch (genfit::Exception& e) {
                   G4cerr << "GenFit exception: " << e.what() << G4endl;
                   break;
                } catch (std::exception& e) {
                   G4cerr << "Standard exception: " << e.what() << G4endl;
                   break;
                } 

                s -= 2.0;
            }
        }
        
    }catch (genfit::Exception& e) {
        G4cerr << "GenFit exception: " << e.what() << G4endl;
    }catch (std::exception& e) {
        G4cerr << "Standard exception: " << e.what() << G4endl;
    }
      
    delete gfTrack; 
}

xyzVector KalmanFit::GetPointOfClosestApproach(xyzVector wireEnd1, xyzVector trackPoint, xyzVector wireVec){
    
    // Vector from the wire start to the track point
    xyzVector diff = {
        trackPoint.X() - wireEnd1.X(), 
        trackPoint.Y() - wireEnd1.Y(), 
        trackPoint.Z() - wireEnd1.Z()
    };
    
    double dotProduct = diff.X()*wireVec.X() + diff.Y()*wireVec.Y() + diff.Z()*wireVec.Z();
    double magSquared = wireVec.X()*wireVec.X() + wireVec.Y()*wireVec.Y() + wireVec.Z()*wireVec.Z();
        
    if (magSquared == 0.0) magSquared = 1e-32; // Prevent division by 0
    
    double constant = dotProduct/magSquared;
        
    xyzVector closest = {
        wireEnd1.X() + constant * wireVec.X(), 
        wireEnd1.Y() + constant * wireVec.Y(), 
        wireEnd1.Z() + constant * wireVec.Z()
    };
        
    return closest;
}

// Finds the position of the detector wire that hit would have come from
HitData KalmanFit::GetClosestWire(LayerHit layer){
    HitData closestWires;
    
    // Entry and exit radii in x,y plane
    double r1 = std::hypot(layer.entryPos.X(), layer.entryPos.Y());
    double r2 = std::hypot(layer.exitPos.X(), layer.exitPos.Y());
            
    // Midpoint radius
    double r = 0.5 * (r1 + r2);
    
    // A track can only hit a max of three cells in a layer so test three points
    xyzVector p1 = layer.entryPos;
    xyzVector p2 = layer.exitPos;
    xyzVector p3 = {(p1.X() + p2.X())/2, (p1.Y() + p2.Y())/2, (p1.Z() + p2.Z())/2};
    
    int layerIndex = layer.layerID - 20000;
    auto [numWires, wireLength, type] = numWiresPerLayer[layerIndex];
    double delta = TWOPI / static_cast<double>(numWires);
        
    // Track the absolute best wire positions found so far
    xyzVector bestP1Wire, bestP2Wire, bestP3Wire;
    std::pair<xyzVector, xyzVector> endsP1, endsP2, endsP3;
    double minDistP1 = 1e9; 
    double minDistP2 = 1e9;
    double minDistP3 = 1e9;
    
    // Alternate layers are staggered by half a cell
    double offset = (layerIndex % 2 == 0) ? 0.0 : delta / 2.0;
    
    for (int i = 0; i < numWires; i++){
        double t1 = offset + (i + 0.5) * delta;
        double t2;
            
        if (type == "stereo+") {
            t2 = offset + (i + 3.5) * delta; // Skew 3 wire places forward
        } else if (type == "stereo-") {
            t2 = offset + (i - 2.5) * delta; // Skew 3 wire places backward
        } else {
            t2 = t1; // Axial wire so straight across
        }
        
        xyzVector end1 = {r * std::cos(t1), r * std::sin(t1), -wireLength / 2.0};
        xyzVector end2 = {r * std::cos(t2), r * std::sin(t2), wireLength / 2.0};      
        xyzVector wireVec = {end2.X()-end1.X(), end2.Y()-end1.Y(), end2.Z()-end1.Z()};
        
        // Find the absolute closest point on this wire segment
        xyzVector p1ClosestOnWire = GetPointOfClosestApproach(end1, p1, wireVec);
        xyzVector p2ClosestOnWire = GetPointOfClosestApproach(end1, p2, wireVec);
        xyzVector p3ClosestOnWire = GetPointOfClosestApproach(end1, p3, wireVec);
        
        // Get distances between track points and the wire segment
        double d1 = GetDistance(p1, p1ClosestOnWire);
        double d2 = GetDistance(p2, p2ClosestOnWire);
        double d3 = GetDistance(p3, p3ClosestOnWire);
            
        std::pair<xyzVector, xyzVector> endPoints = std::make_pair(end1, end2);
            
        if (d1 < minDistP1) { minDistP1 = d1; bestP1Wire = p1ClosestOnWire; endsP1 = endPoints;}
        if (d2 < minDistP2) { minDistP2 = d2; bestP2Wire = p2ClosestOnWire; endsP2 = endPoints;}
        if (d3 < minDistP3) { minDistP3 = d3; bestP3Wire = p3ClosestOnWire; endsP3 = endPoints;}
    }
    
    // Keep only the different cells the track passes through
    closestWires.push_back(std::make_pair(bestP1Wire, endsP1));

    if (!AreVectorsEqual(bestP2Wire, bestP1Wire)) {
        closestWires.push_back(std::make_pair(bestP2Wire, endsP2));
    }
    
    if (!AreVectorsEqual(bestP3Wire, bestP1Wire) && !AreVectorsEqual(bestP3Wire, bestP2Wire)) {
        closestWires.push_back(std::make_pair(bestP3Wire, endsP3));
    }

    return closestWires;
}

// Reads in data about each wire layer in the detector output by the sim
std::map<int, std::tuple<int, double, std::string>> KalmanFit::GetWiresPerLayer(){
    std::ifstream file("layer_radius.csv");
    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file.\n";
        return {};
    }

    std::map<int, std::tuple<int, double, std::string>> wiresMap;
    std::string line;
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string index, numWires, wireLength, type, extra;

        // Parse columns separated by commas
        if (std::getline(ss, index, ',') &&
            std::getline(ss, numWires, ',') &&
            std::getline(ss, wireLength, ',') &&
            std::getline(ss, type, ',') &&
            std::getline(ss, extra)) { 
            
            wiresMap[std::stoi(index)] = std::make_tuple(std::stoi(numWires), 
                                                         std::stod(wireLength),
                                                         type);
        }
    }

    file.close();
    return wiresMap;
}

// Makes a ixj matrix of data read from a csv file
IndexMap KalmanFit::GetLookupTable(std::string fileName){
    std::ifstream file(fileName);
    std::string line;
    IndexMap lookupTable;
    
    // Read file line by line
    while (std::getline(file, line)) {
        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string cell;

        // Split the line by commas
        while (std::getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        
        lookupTable.push_back(row);
    }
    
    file.close();
    return lookupTable;
}

// Returns the helix of the track across the cell
HelixApproach KalmanFit::GetHelix(CellHit cell){
    xyzVector origin = cell.wirePos;
    G4ThreeVector entryPoint = {cell.entry.X() - origin.X(),
                                cell.entry.Y() - origin.Y(),
                                cell.entry.Z() - origin.Z()};  
                                    
    G4ThreeVector entryMom = {cell.entryMom.X(), cell.entryMom.Y(), cell.entryMom.Z()};
    G4ThreeVector magField = {BField.X(), BField.Y(), BField.Z()};
        
    HelixApproach helix(entryPoint, entryMom, magField, particleMass, particleCharge);
    return helix;
}

// Check the distance between two points
double KalmanFit::GetDistance(xyzVector pA, xyzVector pB){
    double dx = pA.X() - pB.X();
    double dy = pA.Y() - pB.Y();
    double dz = pA.Z() - pB.Z();
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

// Check if two vectors are the same
bool KalmanFit::AreVectorsEqual(xyzVector pA, xyzVector pB){
    return (pA.X() == pB.X() && pA.Y() == pB.Y() && pA.Z() == pB.Z());
}

}; // namespace


int main(int argc, char** argv) {
    std::cout << "============= Kalman Filter Processing ===========" << std::endl;
    
    DriftChamberSim::KalmanFit fitter;
    fitter.ProcessParticleTracks();
    
    std::cout << "==================================================" << std::endl;
    return 0;
}
