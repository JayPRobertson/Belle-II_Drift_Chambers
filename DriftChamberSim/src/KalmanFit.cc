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

using Point = DriftChamberSim::KalmanFit::Point;
using Intersection = DriftChamberSim::KalmanFit::Intersection;
using CellCrossing = DriftChamberSim::KalmanFit::CellCrossing;
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
    TTreeReaderValue<TrackedParticle> particle(reader, "Particle");            
    
    // Process input data
    while (reader.Next()) {
        int trackID = particle->getID();
            
        xyzVector initMom = {particle->getMomentum().X(), 
                             particle->getMomentum().Y(), 
                             particle->getMomentum().Z()};
        momMap[trackID] = initMom;
 
        for (const auto& seg : particle->getTrackSegments()) {
            LayerHit hit;
            hit.entryPos = {seg.getEntryPosition().X(), 
                            seg.getEntryPosition().Y(), 
                            seg.getEntryPosition().Z()};
            hit.initMom = { seg.getEntryMomentum().X(), 
                            seg.getEntryMomentum().Y(), 
                            seg.getEntryMomentum().Z()};
            hit.exitPos = { seg.getExitPosition().X(), 
                            seg.getExitPosition().Y(), 
                            seg.getExitPosition().Z()};
            hit.postMom = { seg.getExitMomentum().X(), 
                            seg.getExitMomentum().Y(), 
                            seg.getExitMomentum().Z()};
                                
            hit.entryTime = seg.getEntryTime();
            hit.exitTime = seg.getExitTime();
            hit.edep = seg.getEnergyLoss();
            hit.layerID = seg.getLayerIndex();
            hit.trackID = trackID;
            
            trackMap[trackID].push_back(hit);
            actualHits.push_back(hit);
        }
    }
    
    // Initialize GenFit
    std::cout << "Initializing GenFit field" << std::endl;
    genfit::FieldManager::getInstance()->init(new genfit::ConstField(BField.X() *1.e4,
                                                                     BField.Y() *1.e4,
                                                                     BField.Z() *1.e4)); 
    genfit::MaterialEffects::getInstance()->setNoEffects();
    
    IndexMap timeTable = GetLookupTable("drift_times_lookup.csv");
    IndexMap diffusionTable = GetLookupTable("diffusion_lookup.csv");
    
    // Setup outfile
    std::ofstream chiFile("chi2.csv");
    chiFile << "chi2,ndf\n";
    chiFile.close();
    
    int count = 0;
    
    // Perform fits and write out complete rows
    for (auto& pair : trackMap) {
        int i = pair.first;
        auto& hits = pair.second;

        std::sort(hits.begin(), hits.end(), 
           [](const LayerHit& a, const LayerHit& b) {
                return a.layerID < b.layerID;
        });

        std::vector<CellHit> detectedCells = GetDetectedWires(hits);
        
        if (count == 2){
            GetClusterInfo(detectedCells, timeTable, diffusionTable);
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
        
        // Entry and exit positions and radii
        const xyzVector& entryPos = layerHit.entryPos;
        const xyzVector& exitPos  = layerHit.exitPos;

        double r1 = std::hypot(entryPos.X(), entryPos.Y());
        double r2 = std::hypot(exitPos.X(), exitPos.Y());
            
        // Midpoint radius
        double r = 0.5 * (r1 + r2);

        int layerIndex = layerHit.layerID - 20000;

        auto it = numWiresPerLayer.find(layerIndex);
        if (it == numWiresPerLayer.end()) {
            std::cerr << "Warning: no wire count for layer "
                      << layerIndex << std::endl;
            continue;
        }

        int n = it->second;
        double delta = TWOPI / static_cast<double>(n);

        // Alternate layers are staggered by half a cell.
        double offset = (layerIndex % 2 == 0) ? 0.0 : delta / 2.0;

        for (int i = 0; i < n; ++i) {

            double t1 = offset + i * delta;
            double t2 = offset + (i + 1) * delta;

            double thetaMin = std::atan2(std::sin(t1), std::cos(t1));

            double thetaMax = std::atan2(std::sin(t2), std::cos(t2));

            CellCrossing crossing = IsInCell(layerHit, r1, r2, thetaMin, thetaMax);

            if (!crossing.crossed)
                continue;

            double tc = offset + (i + 0.5) * delta; // Cell centre

            double wireX = r * std::cos(tc);
            double wireY = r * std::sin(tc);
            double wireZ = 0.5 * (entryPos.Z() + exitPos.Z());

            xyzVector wirePosition(wireX, wireY, wireZ);

            detectedCells.push_back({
                wirePosition,
                layerHit.initMom,
                crossing.length,
                layerIndex,
                i,
                crossing.entry,
                crossing.exit,
                t1,
                t2
            });

            break;
        }
    }

    return detectedCells;
}

void KalmanFit::GetClusterInfo(std::vector<CellHit> detectedCells, IndexMap timeTable, IndexMap diffusionTable){
    auto ranInstance = RandomGenerator::instance();
    
    // Look at the track segment through each cell it hits
    for (auto cell : detectedCells){
        int numClusters = ranInstance.fromPoisson(cell.length * avgNumClusters);
        
        HelixApproach helix = GetHelix(cell);
        
        // Randomly generate clusters along the track in the cur cell
        for (int i=0; i < numClusters; i++){
            
            // Find position along helix at time ut
            double ut = ranInstance.fromUniform(0., cell.t2-cell.t1); 
            G4ThreeVector clusterOrigin = helix.Position(ut);
            
            int curi = static_cast<int>(std::round(clusterOrigin.x()));
            int curj = static_cast<int>(std::round(clusterOrigin.y()));
            
            // Make positive index starting at 0
            curi *= (curi > 0) ? 2 : -1;
            curj *= (curj > 0) ? 2 : -1;
            
            // Get the two diffusion values stored in the lookup table string
            std::string diffusion = diffusionTable[curi][curj];
            if (diffusion.empty()) continue;

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
        }
    }
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
                  << " too many or too few hits " << detectedCells.size() 
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

    int hitID = 0;
    const double wireHalfLength = 1000.; //Arbitrary length

    for (const auto& cell : detectedCells) {
                                  
        // Convert positions to [cm]
        const double wireX = cell.wirePos.X() / 10.; 
        const double wireY = cell.wirePos.Y() / 10.;
        const double wireZ = cell.wirePos.Z() / 10.;
        
         // Find shortest drift distance between track in the cell and wire
        HelixApproach helix = GetHelix(cell);
        double increment = (cell.t2 - cell.t1)/1000.;
        double driftDistance = 1000.; 
        bool isIncreasing = false;
        
        for (double t = cell.t1; t < cell.t2; t+=increment){
            G4ThreeVector pos = helix.Position(t);
            
            double curDistance = std::hypot(pos.x()/10. - wireX, 
                                            pos.y()/10. - wireY,
                                            pos.z()/10. - wireZ);
                
            if (curDistance < driftDistance){
                driftDistance = curDistance;
            }else{
                isIncreasing = true;
            }
            if (isIncreasing) break;
        }
        
        const double driftDistanceError = 1.0 * driftDistance;
        
        TVector3 endPoint1(wireX, wireY, wireZ - wireHalfLength);
        TVector3 endPoint2(wireX, wireY, wireZ + wireHalfLength);

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
        
        // Write out chi2 data        
        std::ofstream chiFile("chi2.csv", std::ios_base::app);
        chiFile << status->getChi2() << "," << status->getNdf() << "\n";
        chiFile.close();

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
    G4ThreeVector entryPoint = {cell.entry.x - origin.X(),
                                cell.entry.y - origin.Y(),
                                cell.entry.z - origin.Z()};  
                                    
    G4ThreeVector entryMom = {cell.initMom.X(), cell.initMom.Y(), cell.initMom.Z()};
    G4ThreeVector magField = {BField.X(), BField.Y(), BField.Z()};
        
    HelixApproach helix(entryPoint, entryMom, magField, particleMass, particleCharge);
    return helix;
}

bool KalmanFit::IsAngleInRange(double theta, double thetaMin, double thetaMax){
    auto wrap = [](double a) { return std::remainder(a, DriftChamberSim::TWOPI); };

    theta = wrap(theta);
    thetaMin = wrap(thetaMin);
    thetaMax = wrap(thetaMax);

    double span = wrap(thetaMax - thetaMin);
    if (span < 0) span += DriftChamberSim::TWOPI;

    double rel = wrap(theta - thetaMin);
    if (rel < 0)  rel += DriftChamberSim::TWOPI;

    return rel <= span;
}

bool KalmanFit::IsPointInRegion(Point p, double rMin, double rMax, double thetaMin, double thetaMax){
    double r2 = p.x*p.x + p.y*p.y;
    if (r2 < rMin * rMin || r2 > rMax * rMax) return false;
    
    double theta = std::atan2(p.y, p.x);
    return IsAngleInRange(theta, thetaMin, thetaMax);
}

bool KalmanFit::OnSegment(Point p, Point q, Point r) {
    return q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) &&
           q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y);
}

// Check line segments intersect
Intersection KalmanFit::IsIntersect(Point a, Point b, Point c, Point d) {
    double dx1 = b.x - a.x;
    double dy1 = b.y - a.y;
    double dx2 = d.x - c.x;
    double dy2 = d.y - c.y;
    double determinant = dx1 * dy2 - dy1 * dx2;

    // If lines are parallel or coincident
    if (std::abs(determinant) < 1e-12) return {};

    double t = ((c.x - a.x) * dy2 - (c.y - a.y) * dx2) / determinant;
    double u = ((c.x - a.x) * dy1 - (c.y - a.y) * dx1) / determinant;

    if (t < 0.0 || t > 1.0)  return {};
    if (u < 0.0 || u > 1.0)  return {};

    Point intersection {a.x + t * dx1, a.y + t * dy1};
    return {true, t, intersection};
}

std::vector<Intersection> KalmanFit::IsIntersectArc(
        Point p1, Point p2, double R, double thetaMin, double thetaMax){

    std::vector<Intersection> intersections;

    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    double A = dx*dx + dy*dy;

    if (A < 1e-12) return intersections;

    double B = 2.0 * (p1.x * dx + p1.y * dy);
    double C = p1.x*p1.x + p1.y*p1.y - R*R;

    double discriminant = B*B - 4.0 * A * C;
    if (discriminant < 0.0) return intersections;
    double sqrtDisc = std::sqrt(discriminant);

    std::vector<double> tValues = {
        (-B - sqrtDisc) / (2.0 * A),
        (-B + sqrtDisc) / (2.0 * A)
    };

    for (double t : tValues){
        if (t < 0.0 || t > 1.0) continue;

        Point p { p1.x + t * dx, p1.y + t * dy };

        if (IsAngleInRange(std::atan2(p.y, p.x), thetaMin, thetaMax)) {
            intersections.push_back({true, t, p});
        }
    }
    
    return intersections;
}

CellCrossing KalmanFit::IsInCell(LayerHit hit, double rMin, 
                      double rMax, double thetaMin, double thetaMax){

    std::vector<Intersection> intersections;
    
    Point p1{hit.entryPos.X(), hit.entryPos.Y(), hit.entryPos.Z()};
    Point p2{hit.exitPos.X(),  hit.exitPos.Y(), hit.exitPos.Z()};

    // Add start and end points if  already inside the cell
    if (IsPointInRegion(p1, rMin, rMax, thetaMin, thetaMax)){
        intersections.push_back({true, 0.0, p1});
    }
    if (IsPointInRegion(p2, rMin, rMax, thetaMin,thetaMax)){
        intersections.push_back({true, 1.0, p2});
    }

    // Check intersections with the two radial boundaries
    Intersection radialMin = IsIntersect( p1, p2,
            {rMin * std::cos(thetaMin),
             rMin * std::sin(thetaMin)},
            {rMax * std::cos(thetaMin),
             rMax * std::sin(thetaMin)}
        );

    Intersection radialMax = IsIntersect( p1, p2,
            {rMin * std::cos(thetaMax),
             rMin * std::sin(thetaMax)},
            {rMax * std::cos(thetaMax),
             rMax * std::sin(thetaMax)}
        );

    if(radialMin.valid)  intersections.push_back(radialMin);
    if(radialMax.valid)  intersections.push_back(radialMax);

    // Check intersections with the two circular boundaries
    auto innerArc = IsIntersectArc(p1, p2, rMin, thetaMin,thetaMax);
    auto outerArc = IsIntersectArc(p1, p2, rMax,thetaMin, thetaMax);

    for(auto& x : innerArc)
        intersections.push_back(x);

    for(auto& x : outerArc)
        intersections.push_back(x);

    if(intersections.size() < 2) return {};

    // Sort all intersections along the particle trajectory
    std::sort(intersections.begin(), intersections.end(),
           [](const Intersection& a, const Intersection& b){
                return a.t < b.t;
            });

    Point entry = intersections.front().p;
    Point exit  = intersections.back().p;

    xyzVector initMom = hit.initMom;
    G4ThreeVector initMomVec(initMom.x(), initMom.y(), initMom.z());
    
    HelixApproach helix( initMomVec, particleMass );
    double length = helix.TrackLength(hit.entryTime, hit.exitTime);

    return { true, entry, exit, length};
}

std::map<int, int> KalmanFit::GetWiresPerLayer(){
    std::ifstream file("layer_radius.csv");
    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file.\n";
        return {};
    }

    std::map<int, int> wiresMap;
    std::string line;
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string index, numWires, extra;

        // Parse columns separated by commas
        if (std::getline(ss, index, ',') &&
            std::getline(ss, numWires, ',') &&
            std::getline(ss, extra)) { 
            
            wiresMap[std::stoi(index)] = std::stoi(numWires);
        }
    }

    file.close();
    return wiresMap;
}

}; // namespace


int main(int argc, char** argv) {
    std::cout << "============= Kalman Filter Processing ===========" << std::endl;
    
    DriftChamberSim::KalmanFit fitter;
    fitter.ProcessParticleTracks();
    
    std::cout << "==================================================" << std::endl;
    return 0;
}
