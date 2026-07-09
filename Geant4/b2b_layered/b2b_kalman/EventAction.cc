#include "EventAction.hh"

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <exception>

#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4Event.hh"
#include "G4TrajectoryContainer.hh"
#include "globals.hh"
#include "TrackerHit.hh"
#include "G4SDManager.hh"
#include "HelixApproach.hh"

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

    for (auto& entry : tracks) {
        int trackID = entry.first;
        auto& hits = entry.second;

        if (hits.size() < 5 || trackID != 1) continue;

        std::sort(hits.begin(), hits.end(), [](TrackerHit* a, TrackerHit* b){
            return a->GetChamberNb() < b->GetChamberNb();
        });

        // Keep only one measurement per detector layer
        std::map<int, TrackerHit*> bestHits;
        
        for(auto* hit : hits){
            int layer = hit->GetChamberNb();
            auto it = bestHits.find(layer);
        
            if(it == bestHits.end()){
                bestHits[layer] = hit;
            }else if(hit->GetEdep() > it->second->GetEdep()){
                bestHits[layer] = hit;
            }
        }
        
        hits.clear();
        
        for(auto& h : bestHits){
            hits.push_back(h.second);
        }
        
        std::sort(hits.begin(), hits.end(), [](TrackerHit* a, TrackerHit* b){
                return a->GetChamberNb() < b->GetChamberNb();
            });
        
        
        if(hits.size() > 300 || hits.size() < 5){
            G4cout << "Skipping track " << trackID 
                   << " too many or too few hits " << hits.size() << G4endl;
        
            continue;
        }

        TrackerHit* firstHit = hits.front();
        G4ThreeVector p0 = firstHit->GetPos();
        
        TVector3 initPos(p0.x()/10., p0.y()/10.,p0.z()/10.); //[cm]
        
        G4ThreeVector p1 = hits[0]->GetPos();
        G4ThreeVector p2 = hits[std::min((size_t)5,hits.size()-1)]->GetPos();
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
        for (auto* hit : hits){
            G4ThreeVector pos = hit->GetPos();
            TVectorD coords(3);
            coords[0] = pos.x()/10.; //[cm]
            coords[1] = pos.y()/10.; //[cm]
            coords[2] = pos.z()/10.; //[cm]

            // Measurement uncertainties
            double sigmaXY = 0.05; //[cm]
            double sigmaZ  = 0.05;  //[cm]
            
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
                   << ", hits = " << hits.size()
                   << ", fitted = " << status->isFitted()
                   << ", converged = " << status->isFitConverged()
                   << ", chi2 = " << status->getChi2()
                   << ", ndf = " << status->getNdf() << G4endl;

            if (status->isFitConverged()) {
                genfit::MeasuredStateOnPlane state = gfTrack->getFittedState();
                
                G4Polyline predictedTrack;

                for (double s = -300.0; s <= 300.0; s += 1.0) {
                    genfit::MeasuredStateOnPlane sampleState(state);
                
                    try {
                        rep->extrapolateBy(sampleState, s);
                        TVector3 pos = sampleState.getPos();
                
                        predictedTrack.push_back(
                            G4Point3D(pos.X() * cm, pos.Y() * cm, pos.Z() * cm )
                        );
                        
                    }catch (genfit::Exception& e) {
                        G4cerr << "GenFit exception: " << e.what() << G4endl;
                    }catch (std::exception& e) {
                        G4cerr << "Standard exception: " << e.what() << G4endl;
                    }           
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
