#include <iostream>
#include <fstream>
#include <sstream>

#include <TGenericClassInfo.h>
#include <TFile.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>

#include <TVector3.h>
#include <TVectorD.h>
#include <TMatrixDSym.h>

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

#include <SpacepointMeasurement.h>
#include <Track.h>
#include <RKTrackRep.h>
#include <KalmanFitterRefTrack.h>
#include <MaterialEffects.h>
#include <FieldManager.h>
#include <ConstField.h>

#include "KalmanFit.hh"
#include "TrackedParticle.hh"

using Point = B2::KalmanFit::Point;
using Intersection = B2::KalmanFit::Intersection;
using CellCrossing = B2::KalmanFit::CellCrossing;
using CellHit = B2::KalmanFit::CellHit;
using LayerHit = B2::KalmanFit::LayerHit;

namespace B2 {
    
const double TWOPI = 2. * M_PI;
std::map<int, int> numWiresPerLayer;

void KalmanFit::ProcessParticleTracks(){
    
    std::string filepath = "~/Desktop/Belle II/Python/root/";
    
    // Open ROOT file
    TFile *file = TFile::Open((filepath + "particle_and_track_data.root").c_str());
    if (!file || file->IsZombie()) {
        std::cerr << "Error opening file particle_and_track_data.root" << std::endl;
        return;
    }
    
    // Initialize GenFit
    std::cout << "Initializing GenFit field" << std::endl;
    genfit::FieldManager::getInstance()->init(new genfit::ConstField(0,0,15.0));
    genfit::MaterialEffects::getInstance()->setNoEffects();
    
    // Get number of wires in each layer of detector
    numWiresPerLayer = GetWiresPerLayer();
    if (!numWiresPerLayer.size()) return;
    
    TTree* rootTree = (TTree*)file->Get("Particles");
    TTreeReader reader(rootTree);
    TTreeReaderValue<TrackedParticle> particle(reader, "Particle");
    
    std::map<int, std::vector<LayerHit>> trackMap;
    std::map<int, xyzVector> momMap;
    
    // Create a map of track ids and vectors with layer hit data
    while (reader.Next()) {
        int trackID = particle->getID();
            
        xyzVector initMom = {particle->getMomentum().X(), particle->getMomentum().Y(), particle->getMomentum().Z()};
        momMap[trackID] = initMom;
 
        for (const auto& seg : particle->getTrackSegments()) {
            LayerHit hit;
            
            hit.entryPos = {seg.getEntryPosition().X(), seg.getEntryPosition().Y(), seg.getEntryPosition().Z()};
            
            hit.initMom = {seg.getEntryMomentum().X(), seg.getEntryMomentum().Y(), seg.getEntryMomentum().Z()};
            
            hit.exitPos = {seg.getExitPosition().X(), seg.getExitPosition().Y(), seg.getExitPosition().Z()};
            
            hit.postMom = {seg.getExitMomentum().X(), seg.getExitMomentum().Y(), seg.getExitMomentum().Z()};
            
            hit.entryTime = seg.getEntryTime();
            hit.exitTime = seg.getExitTime();
            hit.edep = seg.getEnergyLoss();
            hit.layerID = seg.getLayerIndex();
            
            trackMap[trackID].push_back(hit);
        }
    }
    
    for (auto& pair : trackMap) {
            int i = pair.first;
            auto& hits = pair.second;

            // Sort by increasing layerID
            std::sort(hits.begin(), hits.end(), [](const LayerHit& a, const LayerHit& b) {
                return a.layerID < b.layerID;
            });

            std::vector<xyzVector> detectedWirePos = GetDetectedWires(hits);
            GetKalmanFit(detectedWirePos, i, momMap[i]);
    }
    
    file->Close();
}

std::vector<xyzVector> KalmanFit::GetDetectedWires(std::vector<LayerHit> sortedHits){
    std::vector<xyzVector> detectedWirePos;
    std::vector<CellHit> detectedCells;
  
    for (const auto& layerHit : sortedHits) {
        
        // Entry and exit positions and radii
        xyzVector entryPos = layerHit.entryPos;
        xyzVector exitPos  = layerHit.exitPos;
        double r1 = std::hypot(entryPos.X(), entryPos.Y());
        double r2 = std::hypot(exitPos.X(), exitPos.Y());
    
        // Midpoint radius
        double r = 0.5 * (r1 + r2);
            
        int layerIndex = layerHit.layerID - 20000;
        int n = numWiresPerLayer[layerIndex];
        double delta = B2::TWOPI / static_cast<double>(n);
        
        bool cellAdded = false;
        int cellCount = 0;
    
        // Alternate layers are staggered by half a cell
        double offset = (layerIndex % 2 == 0) ? 0.0 : delta / 2.0;
    
        Point p1{entryPos.X(), entryPos.Y()};
        Point p2{exitPos.X(),  exitPos.Y()};
    
        for (int i = 0; i < n; ++i){  
            double t1 = offset + i * delta;
            double t2 = offset + (i + 1) * delta;
        
            double thetaMin = std::atan2(std::sin(t1), std::cos(t1));
            double thetaMax = std::atan2(std::sin(t2), std::cos(t2));
        
            CellCrossing crossing = isInCell(p1, p2, r1, r2, thetaMin, thetaMax);
        
            if(crossing.crossed){
                double tc = offset + (i + 0.5) * delta; // Cell centre
                
                double wireX = r * std::cos(tc);
                double wireY = r * std::sin(tc);
                double wireZ = 0.5 * (entryPos.Z()+exitPos.Z());
        
                xyzVector wirePosition(wireX, wireY, wireZ);
        
                detectedCells.push_back({
                        wirePosition,
                        crossing.length,
                        layerIndex,
                        i,
                        crossing.entry,
                        crossing.exit
                });
        
                if(!cellAdded){
                    detectedWirePos.push_back(wirePosition);
                    cellAdded = true;
                }
                cellCount++;
                
                //std::cout << "      Layer = " << layerIndex
                          //<< ", cell = " << i
                          //<< ", length = " << crossing.length << " mm" 
                          //<< ", clusters = " << crossing.length * avgNumClusters
                          //<< std::endl;
                       
            } else if(cellAdded){
                break;
            }
        }
    }
    
    return detectedWirePos;
}

void KalmanFit::GetKalmanFit(std::vector<xyzVector> detectedWirePos, int trackID, xyzVector initMomentum){
    
    std::sort(detectedWirePos.begin(), detectedWirePos.end(), 
         [](const xyzVector& v1, const xyzVector& v2) {
                return v1.R() < v2.R(); 
    });
        
    if(detectedWirePos.size() > 300 || detectedWirePos.size() < 5){
        std::cout << "Skipping track " << trackID 
                  << " too many or too few hits " << detectedWirePos.size() 
                  << std::endl;
    
        return;
    }

    xyzVector p1 = detectedWirePos.front();
    
    TVector3 initPos(p1.X()/10., p1.Y()/10., p1.Z()/10.); //[cm]
    size_t directionIndex = std::min((size_t)5, detectedWirePos.size()-1);
    
    xyzVector p2 = detectedWirePos[directionIndex];
    xyzVector direction = (p2-p1).unit();
    
    double momentum = initMomentum.R()/1000.0; //[GeV]

    TVector3 initMom(
        direction.X()*momentum,
        direction.Y()*momentum,
        direction.Z()*momentum
    );

    
    auto* rep = new genfit::RKTrackRep(13);
    auto* gfTrack = new genfit::Track(rep, initPos, initMom);

    int hitID = 0;
    for (auto pos : detectedWirePos){
        TVectorD coords(3);
        coords[0] = pos.X()/10.; //[cm]
        coords[1] = pos.Y()/10.; //[cm]
        coords[2] = pos.Z()/10.; //[cm]

        // Measurement uncertainties
        double sigmaXY = 0.5;
        double sigmaZ = 0.5; 
        
        TMatrixDSym cov(3);
        cov.Zero();
        
        cov(0,0)=sigmaXY*sigmaXY;
        cov(1,1)=sigmaXY*sigmaXY;
        cov(2,2)=sigmaZ*sigmaZ;

        auto* measurement = new genfit::SpacepointMeasurement(
            coords, cov, 0, hitID, nullptr );

        gfTrack->insertMeasurement(measurement);
        hitID++;
    }

    genfit::KalmanFitterRefTrack fitter;
    fitter.setMaxIterations(100);

    try {
        fitter.processTrack(gfTrack);
        auto* status = gfTrack->getFitStatus(rep);
        
        // Print out fitting status
        std::cout << ">>> Track = " << trackID
                  << ", hits = " << detectedWirePos.size()
                  << ", fitted = " << status->isFitted()
                  << ", converged = " << status->isFitConverged()
                  << ", chi2 = " << status->getChi2()
                  << ", ndf = " << status->getNdf() << std::endl;

        if (status->isFitConverged()) {
            genfit::MeasuredStateOnPlane state = gfTrack->getFittedState();
            
            // Print Kalman fitting data
            G4cout << "       Momentum = " << state.getMom().Mag() << " GeV" << G4endl;
            G4cout << "       Position = " << state.getPos().X() << " "
                                    << state.getPos().Y() << " "
                                    << state.getPos().Z() << " cm" << G4endl;
        }  
        
    }catch (genfit::Exception& e) {
        G4cerr << "GenFit exception: " << e.what() << G4endl;
    }catch (std::exception& e) {
        G4cerr << "Standard exception: " << e.what() << G4endl;
    }
    
    delete gfTrack;
}

bool KalmanFit::isAngleInRange(double theta, double thetaMin, double thetaMax){
    auto wrap = [](double a) { return std::remainder(a, B2::TWOPI); };

    theta     = wrap(theta);
    thetaMin = wrap(thetaMin);
    thetaMax = wrap(thetaMax);

    double span = wrap(thetaMax - thetaMin);
    if (span < 0) span += B2::TWOPI;

    double rel = wrap(theta - thetaMin);
    if (rel < 0)  rel += B2::TWOPI;

    return rel <= span;
}

bool KalmanFit::isPointInRegion(Point p, double rMin, double rMax, double thetaMin, double thetaMax){
    double r2 = p.x*p.x + p.y*p.y;
    if (r2 < rMin * rMin || r2 > rMax * rMax) return false;
    
    double theta = std::atan2(p.y, p.x);
    return isAngleInRange(theta, thetaMin, thetaMax);
}

bool KalmanFit::onSegment(Point p, Point q, Point r) {
    return q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) &&
           q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y);
}

// Check line segments intersect
Intersection KalmanFit::isIntersect(Point a, Point b, Point c, Point d) {
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

std::vector<Intersection> KalmanFit::isIntersectArc(
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

        if (isAngleInRange(std::atan2(p.y, p.x), thetaMin, thetaMax)) {
            intersections.push_back({true, t, p});
        }
    }
    
    return intersections;
}

CellCrossing KalmanFit::isInCell(Point p1, Point p2, double rMin, 
                      double rMax, double thetaMin, double thetaMax){

    std::vector<Intersection> intersections;

    // Add start and end points if  already inside the cell
    if (isPointInRegion(p1, rMin, rMax, thetaMin, thetaMax)){
        intersections.push_back({true, 0.0, p1});
    }
    if (isPointInRegion(p2, rMin, rMax, thetaMin,thetaMax)){
        intersections.push_back({true, 1.0, p2});
    }

    // Check intersections with the two radial boundaries
    Intersection radialMin = isIntersect( p1, p2,
            {rMin * std::cos(thetaMin),
             rMin * std::sin(thetaMin)},
            {rMax * std::cos(thetaMin),
             rMax * std::sin(thetaMin)}
        );

    Intersection radialMax =
        isIntersect( p1, p2,
            {rMin * std::cos(thetaMax),
             rMin * std::sin(thetaMax)},
            {rMax * std::cos(thetaMax),
             rMax * std::sin(thetaMax)}
        );

    if(radialMin.valid)  intersections.push_back(radialMin);
    if(radialMax.valid)  intersections.push_back(radialMax);

    // Check intersections with the two circular boundaries
    auto innerArc = isIntersectArc(p1, p2, rMin, thetaMin,thetaMax);
    auto outerArc = isIntersectArc(p1, p2, rMax,thetaMin, thetaMax);

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

    double dx = exit.x - entry.x;
    double dy = exit.y - entry.y;
    double length = std::sqrt(dx*dx + dy*dy);

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

}; // namespace b2b


int main(int argc, char** argv) {
    std::cout << "============= Kalman Filter Processing ===========" << std::endl;
    
    B2::KalmanFit fitter;
    fitter.ProcessParticleTracks();
    
    std::cout << "==================================================" << std::endl;
    return 0;
}



