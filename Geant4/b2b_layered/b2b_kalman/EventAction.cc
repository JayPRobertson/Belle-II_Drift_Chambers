#include "EventAction.hh"

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <exception>
#include <cmath>

#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4Event.hh"
#include "G4TrajectoryContainer.hh"
#include "globals.hh"
#include "TrackerHit.hh"
#include "G4SDManager.hh"
#include "HelixApproach.hh"
#include "DetectorConstruction.hh"
#include "G4RunManager.hh"

#include <TVector3.h>
#include <TVectorD.h>
#include <TMatrixDSym.h>

#include <SpacepointMeasurement.h>
#include <Track.h>
#include <RKTrackRep.h>
#include <KalmanFitterRefTrack.h>
#include <MaterialEffects.h>

#include <FieldManager.h>
#include <ConstField.h>

#include "G4Polyline.hh"
#include "G4VisAttributes.hh"
#include "G4VVisManager.hh"
#include "G4Colour.hh"

namespace B2{
    
using Point = EventAction::Point;
  
G4int fTrackerHCID = -1;

void EventAction::BeginOfEventAction(const G4Event*) {
  if (fTrackerHCID == -1) {
        fTrackerHCID = G4SDManager::GetSDMpointer()->GetCollectionID("TrackerHitsCollection");
  }
  
  fTrackedDistance = 0.0;
  
  // Reset tracking variables
  prePosX = "";
  prePosY = "";
  prePosZ = "";
  
  postPosX = "";
  postPosY = "";
  postPosZ = "";
  
  totEdep = "";
  
  fTrackedEdep = 0.0;
  curIndex = -1;
  
  enteredGas = false;
  
  if (!genfitFieldInitialized) {
        G4cout << "Initializing GenFit field" << G4endl;
        
        genfit::FieldManager::getInstance()->init(new genfit::ConstField(0,0,15.0));
        genfit::MaterialEffects::getInstance()->setNoEffects();
        genfitFieldInitialized = true;
  }
  
}


// ________________________ HELPER FUNCTIONS ______________________________

bool isAngleInRange(double theta, double theta_min, double theta_max) {
    auto wrap = [](double a) { return std::remainder(a, CLHEP::twopi); };

    theta     = wrap(theta);
    theta_min = wrap(theta_min);
    theta_max = wrap(theta_max);

    double span = wrap(theta_max - theta_min);
    if (span < 0) span += CLHEP::twopi;

    double rel = wrap(theta - theta_min);
    if (rel < 0)  rel += CLHEP::twopi;

    return rel <= span;
}

bool isPointInRegion(Point p, double r_min, double r_max, double theta_min, double theta_max){
    double r2 = p.x * p.x + p.y * p.y;
    if (r2 < r_min * r_min || r2 > r_max * r_max) return false;
    
    double theta = std::atan2(p.y, p.x);
    return isAngleInRange(theta, theta_min, theta_max);
}

double crossProduct(Point o, Point a, Point b){
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

// Check line segments intersect
bool isIntersect(Point a, Point b, Point c, Point d){
    auto sign = [](double val) { return (val > 1e-9) - (val < -1e-9); };
    
    double cp1 = crossProduct(a, b, c);
    double cp2 = crossProduct(a, b, d);
    double cp3 = crossProduct(c, d, a);
    double cp4 = crossProduct(c, d, b);
    
    return (sign(cp1) * sign(cp2) < 0 && sign(cp3) * sign(cp4) < 0);
}

bool isIntersectArc(Point p1, Point p2, double R, double theta_min, double theta_max){
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;
    double A = dx * dx + dy * dy;
    if (A < 1e-9) return false; 

    double B = 2 * (p1.x * dx + p1.y * dy);
    double C = p1.x * p1.x + p1.y * p1.y - R * R;
    double discriminant = B * B - 4 * A * C;

    if (discriminant < 0) return false;

    double sqrt_disc = std::sqrt(discriminant);
    double t1 = (-B - sqrt_disc) / (2 * A);
    double t2 = (-B + sqrt_disc) / (2 * A);

    // Check intersection points on the segment (t in [0, 1])
    for (double t : {t1, t2}){
        if (t >= 0.0 && t <= 1.0){
            double ix = p1.x + t * dx;
            double iy = p1.y + t * dy;
            if (isAngleInRange(std::atan2(iy, ix), theta_min, theta_max)){
                return true;
            }
        }
    }
    return false;
}

bool isInCell(Point p1, Point p2, double r_min, double r_max, 
                                     double theta_min, double theta_max){
    
    if (isPointInRegion(p1, r_min, r_max, theta_min, theta_max) ||
        isPointInRegion(p2, r_min, r_max, theta_min, theta_max)){
        return true;
    }
    
    // Naming: [r]<min/max>_[theta]<min/max>
    Point min_min = {r_min*std::cos(theta_min), r_min*std::sin(theta_min)};
    Point max_min = {r_max*std::cos(theta_min), r_max*std::sin(theta_min)};
    Point min_max = {r_min*std::cos(theta_max), r_min*std::sin(theta_max)};
    Point max_max = {r_max*std::cos(theta_max), r_max*std::sin(theta_max)};

    if (isIntersect(p1, p2, min_min, max_min) ||
        isIntersect(p1, p2, min_max, max_max)){
        return true;
    }
    
    if (isIntersectArc(p1, p2, r_min, theta_min, theta_max) ||
        isIntersectArc(p1, p2, r_max, theta_min, theta_max)){
        return true;
    }

    return false;
}

// ___________________________________________________________//

void EventAction::EndOfEventAction(const G4Event* event){
    G4int eventID = event->GetEventID();

    if (eventID < 10 || eventID % 100 == 0) {
        G4cout << ">>> Event " << eventID << G4endl;
    }

    auto hce = event->GetHCofThisEvent();
    if (!hce) return;

    auto hc = hce->GetHC(fTrackerHCID);
    if (!hc) return;

    auto hitsCollection = static_cast<TrackerHitsCollection*>(hc);
    if (hitsCollection->GetSize() == 0) return;

    std::map<int, std::vector<TrackerHit*> > tracks;

    for (size_t i = 0; i < hitsCollection->GetSize(); i++) {
        auto* hit = static_cast<TrackerHit*>(hitsCollection->GetHit(i));
        if (!hit) continue;
            
        tracks[hit->GetTrackID()].push_back(hit);
    }
    
    // Get the vector of how many wires are in each layer read in from json
    const G4RunManager* runManager = G4RunManager::GetRunManager();
    const B2b::DetectorConstruction* detectorConstruction = dynamic_cast<const B2b::DetectorConstruction*>(runManager->GetUserDetectorConstruction());
    std::vector<G4int> numWiresPerLayer = detectorConstruction->getNumWires();
      
    for (auto& entry : tracks) {
        int trackID = entry.first;
        auto& hits = entry.second;
        
        if (hits.size() < 5 || trackID != 1) continue;
            
        // Sort hits in ascending order of layer number
        std::sort(hits.begin(), hits.end(), [](TrackerHit* a, TrackerHit* b){
            return a->GetChamberNb() < b->GetChamberNb();
        });

        // Keep the hits as the track enters and exits the layer
        std::map<int, std::pair<TrackerHit*,TrackerHit*>> bestHits; 

        for(auto* hit : hits){ 
            int layer = hit->GetChamberNb(); 
            auto itEntry = bestHits.find(layer); // Get cur layer
            
            // Add layer hit to vector if doesn't exist
            if(itEntry == bestHits.end()){ 
                bestHits[layer] = std::make_pair(hit,hit); 
                
            // Check if cur hit is earlier than entry hit for this layer
            }else if(hit->GetGlobalTime() < itEntry->second.first->GetGlobalTime()){ 
                bestHits[layer].first = hit; 
                
            // Check if cur hit is later than exit hit for this layer
            }else if(hit->GetGlobalTime() > itEntry->second.second->GetGlobalTime()){ 
                bestHits[layer].second = hit; 
            }
        }
        
        hits.clear();
        
        // Get a vector of only the hit pairs
        std::vector<std::pair<TrackerHit*, TrackerHit*>> sortedHits;
        for (const auto& [id, hitPair] : bestHits){
            sortedHits.push_back(hitPair);
        }
        
        // Order the hits by ascending layer radius
        std::sort(sortedHits.begin(), sortedHits.end(), 
            [](const std::pair<TrackerHit*, TrackerHit*>& a, 
               const std::pair<TrackerHit*, TrackerHit*>& b) {
                return a.first->GetChamberNb() < b.first->GetChamberNb(); 
        });
   
        std::vector<G4ThreeVector> detectedWirePos;
        
        for (const auto& eePair : sortedHits) {
            
            // Entry and exit positions and radii
            G4ThreeVector entryPos = eePair.first->GetPos();
            G4ThreeVector exitPos  = eePair.second->GetPos();
            double r1 = std::hypot(entryPos.x(), entryPos.y());
            double r2 = std::hypot(exitPos.x(),  exitPos.y());
        
            // Midpoint radius
            double r = 0.5 * (r1 + r2);
                
            int layerIndex = eePair.first->GetChamberNb() - 20000;
            G4int n = numWiresPerLayer[layerIndex];
            double delta = CLHEP::twopi / static_cast<double>(n);
        
            // Alternate layers are staggered by half a cell
            double offset = (layerIndex % 2 == 0) ? 0.0 : delta / 2.0;
        
            Point p1{entryPos.x(), entryPos.y()};
            Point p2{exitPos.x(),  exitPos.y()};
        
            for (int i = 0; i < n; ++i){  
                double t1 = offset + i * delta;
                double t2 = offset + (i + 1) * delta;
        
                double theta_min = std::atan2(std::sin(t1), std::cos(t1));
                double theta_max = std::atan2(std::sin(t2), std::cos(t2));
        
                if (isInCell(p1, p2, r1, r2, theta_min, theta_max)){
        
                    // Centre of the drift cell
                    double tc = offset + (i + 0.5) * delta;
        
                    double wireX = r * std::cos(tc);
                    double wireY = r * std::sin(tc);
                    double wireZ = 0.5 * (entryPos.z() + exitPos.z());
        
                    detectedWirePos.push_back(G4ThreeVector(wireX, wireY, wireZ));
        
                    // Track currently only crossing one cell in this layer
                    break;
                }
            }
        }
        
        std::sort(detectedWirePos.begin(), detectedWirePos.end(), 
            [](const G4ThreeVector& v1, const G4ThreeVector& v2) {
                return v1.mag() < v2.mag(); 
            });
        
        if(detectedWirePos.size() > 300 || detectedWirePos.size() < 5){
            G4cout << "Skipping track " << trackID 
                   << " too many or too few hits " << detectedWirePos.size() << G4endl;
        
            continue;
        }

        G4ThreeVector p0 = detectedWirePos.front();
        TVector3 initPos(p0.x()/10., p0.y()/10.,p0.z()/10.); //[cm]
        
        G4ThreeVector p1 = detectedWirePos.front();
        G4ThreeVector p2 = detectedWirePos[std::min((size_t)5, hits.size()-1)];
        G4ThreeVector direction = (p2-p1).unit();
        double momentum = initMomentum.mag()/1000.0; //[GeV]
    
        TVector3 initMom(
            direction.x()*momentum,
            direction.y()*momentum,
            direction.z()*momentum
        );
        
        auto* rep = new genfit::RKTrackRep(13);
        auto* gfTrack = new genfit::Track(rep, initPos, initMom);
    
        int hitID = 0;
        for (auto hit : detectedWirePos){
            G4ThreeVector pos = hit;
            TVectorD coords(3);
            coords[0] = pos.x()/10.; //[cm]
            coords[1] = pos.y()/10.; //[cm]
            coords[2] = pos.z()/10.; //[cm]

            // Measurement uncertainties
            double sigmaXY = 0.5; //[cm]
            double sigmaZ  = 0.5;  //[cm]
            
            TMatrixDSym cov(3);
            cov.Zero();
            
            cov(0,0)=sigmaXY*sigmaXY;
            cov(1,1)=sigmaXY*sigmaXY;
            cov(2,2)=sigmaZ*sigmaZ;

            auto* measurement = new genfit::SpacepointMeasurement(
                coords,
                cov,
                0,
                hitID,
                nullptr
            );

            gfTrack->insertMeasurement(measurement);
            hitID++;
        }

        genfit::KalmanFitterRefTrack fitter;
        fitter.setMaxIterations(30);

        try {
            fitter.processTrack(gfTrack);
            auto* status = gfTrack->getFitStatus(rep);
            
            // Print out fitting status
            G4cout << "Track = " << trackID
                   << ", hits = " << detectedWirePos.size()
                   << ", fitted = " << status->isFitted()
                   << ", converged = " << status->isFitConverged()
                   << ", chi2 = " << status->getChi2()
                   << ", ndf = " << status->getNdf() << G4endl;

            if (status->isFitConverged()) {
                genfit::MeasuredStateOnPlane state = gfTrack->getFittedState();
                
                G4Polyline predictedTrack;

                double s = 300.;
                bool passedOrigin = false;
                const double epsilon = 1; 
                
                while (!passedOrigin && std::abs(s) < 800.) {
                    genfit::MeasuredStateOnPlane sampleState(state);
                    try {
                        rep->extrapolateBy(sampleState, s);
                        TVector3 pos = sampleState.getPos();
                        
                        // Check if cur pos is origin
                        if (std::abs(pos.X()) < epsilon && 
                            std::abs(pos.Y()) < epsilon && 
                            std::abs(pos.Z()) < epsilon) {
                            passedOrigin = true;
                        }
                        
                        predictedTrack.push_back(
                            G4Point3D(pos.X() * cm, pos.Y() * cm, pos.Z() * cm)
                        );
                        
                    } catch (genfit::Exception& e) {
                        G4cerr << "GenFit exception: " << e.what() << G4endl;
                        break; // Exit loop if extrapolation fails
                    } catch (std::exception& e) {
                        G4cerr << "Standard exception: " << e.what() << G4endl;
                        break;
                    } 
                    
                    s -= 1.0; 
                }
                
                // Create the Kalman trajectory lines in the Geant4 gui
                G4VisAttributes visAtt(G4Colour::Yellow());
                visAtt.SetLineWidth(3.0);
                
                predictedTrack.SetVisAttributes(visAtt);
                
                if (auto* vis = G4VVisManager::GetConcreteInstance()) {
                    vis->Draw(predictedTrack);
                }
                
                // Print Kalman fitting data
                G4cout << "Momentum = " << state.getMom().Mag() << " GeV" << G4endl;
                G4cout << "Position = " << state.getPos().X() << " "
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
}

}  // namespace B2
