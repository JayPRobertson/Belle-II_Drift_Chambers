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

#include "TrackSegment.hh"
#include "TrackNTupleSvc.hh"
#include <MaterialEffects.h>

#include <FieldManager.h>
#include <ConstField.h>

namespace B2{
  
G4int fTrackerHCID = -1;

void EventAction::BeginOfEventAction(const G4Event*) {
  if (fTrackerHCID == -1) {
        fTrackerHCID = G4SDManager::GetSDMpointer()->GetCollectionID("TrackerHitsCollection");
  }
  
  curIndex = -1;
  fParticleID++;
  enteredGas = false;
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
    
    // Get the vector of how many wires are in each layer read in from json
    const G4RunManager* runManager = G4RunManager::GetRunManager();
    const B2b::DetectorConstruction* detectorConstruction = dynamic_cast<const B2b::DetectorConstruction*>(runManager->GetUserDetectorConstruction());
    
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
        
        for (const auto& hitPair : sortedHits){
            TrackerHit* entryHit = hitPair.first;
            TrackerHit* exitHit = hitPair.second;
            
            G4ThreeVector entryPos = entryHit->GetPos();
            G4ThreeVector exitPos = exitHit->GetPos();
            G4ThreeVector entryMom = entryHit->GetMomentum();
            G4ThreeVector exitMom = exitHit->GetMomentum();
            
            TrackSegment segment(
                     xyzVector{entryPos.x(), entryPos.y(), entryPos.z()},
                     xyzVector{entryMom.x(), entryMom.y(), entryMom.z()},
                     xyzVector{exitPos.x(), exitPos.y(), exitPos.z()},
                     xyzVector{exitMom.x(), exitMom.y(), exitMom.z()}, 
                     entryHit->GetGlobalTime(), 
                     exitHit->GetGlobalTime(), 
                     exitHit->GetEdep(),
                     entryHit->GetChamberNb()
            );
                     
            TrackNTupleSvc::instance().addTrackSegment(fParticleID, segment);  
        }
        
        hits.clear();
    }
}

}  // namespace B2
