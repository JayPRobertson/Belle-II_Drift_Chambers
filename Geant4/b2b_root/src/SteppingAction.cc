#include "SteppingAction.hh"

#include <iostream>
#include <fstream>
#include <string>

#include "G4RunManager.hh"
#include "G4StepPoint.hh"
#include "G4VPhysicalVolume.hh"
#include "EventAction.hh"
#include "G4ThreeVector.hh"

#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4DynamicParticle.hh"
#include <cmath>

#include "G4EmCalculator.hh"
#include "G4ParticleTable.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"

#include "DetectorConstruction.hh"
#include "TrackSegment.hh"
#include "TrackNTupleSvc.hh"
#include "TrackedParticle.hh"

using xyzVector = ROOT::Math::XYZVector;

SteppingAction::SteppingAction(B2::EventAction* eventAction)
 : fEventAction(eventAction){}

void SteppingAction::UserSteppingAction(const G4Step* aStep) {
    
    //_________ Checking if particle is delta ray _________
    
    const G4RunManager* runManager = G4RunManager::GetRunManager();
    const B2b::DetectorConstruction* detectorConstruction = dynamic_cast<const B2b::DetectorConstruction*>(runManager->GetUserDetectorConstruction());
    
    G4Track* track = aStep->GetTrack();
    G4int id = fEventAction->GetParticleID();
    bool isMuon = true;
  
    const G4VProcess* creatorProcess = track->GetCreatorProcess();
    if (creatorProcess) {
      G4String procName = creatorProcess->GetProcessName();
      if (procName.substr(procName.length() - 4) == "Ioni") isMuon = false; 
    }
    
    //_____________ Collect stepping data ______________
    
    G4StepPoint* preStepPoint = aStep->GetPreStepPoint();
    G4StepPoint* postStepPoint = aStep->GetPostStepPoint();
    
    G4VPhysicalVolume* preVol = preStepPoint->GetPhysicalVolume();
    G4VPhysicalVolume* postVol = postStepPoint->GetPhysicalVolume();
    
    G4ThreeVector entryPos = preStepPoint->GetPosition();
    G4ThreeVector exitPos = postStepPoint->GetPosition();
    G4ThreeVector initMomentum = fEventAction->GetInitMomentum();
    
    // Verify particle is inside the volume
    if (!preVol || !postVol) return;
    
    fEventAction->AddLayerEdep(aStep->GetTotalEnergyDeposit());
    G4int postIndex = postVol->GetCopyNo();
        
    // Add new track segment every layer
    if (postIndex%4000 < 1000 && postIndex != preVol->GetCopyNo()){
        
        if (fEventAction->GetGasStatus()){
            G4ThreeVector postMom = postStepPoint->GetMomentum();
            
            // Update instance of current track in ROOT file
            TrackSegment segment(
                     fEventAction->GetLayerEntry(),
                     fEventAction->GetLayerInitMomentum(),
                     xyzVector{exitPos.x(), exitPos.y(), exitPos.z()},
                     xyzVector{postMom.x(), postMom.y(), postMom.z()}, 
                     fEventAction->GetLayerTime(), 
                     fEventAction->GetLayerEdep()
                     );
                     
            TrackNTupleSvc::instance().addTrackSegment(id, segment); 
        }
        
        fEventAction->ResetLayerEdep();
        fEventAction->SetLayerEntryVals( 
                        postStepPoint->GetPosition(),
                        track->GetLocalTime(),
                        postStepPoint->GetMomentum()
                    );
    }

    // Process only if the particle is currently inside a gas layer
    if (preVol->GetName() == "GasLayerRing" && isMuon) {
        fEventAction->AddTrackedDistance(aStep->GetStepLength());
        G4int volumeID = preVol->GetCopyNo();
        G4int curIndex = fEventAction->GetCurIndex();

        // Check if new layer entered
        if (curIndex < volumeID) {
            fEventAction->SetTotEdep(); 

            // Initialize tracking for new layer
            fEventAction->SetCurIndex(volumeID);
            fEventAction->ResetTrackedEdep();
            
            // If was in gas and staying in gas
            if (fEventAction->GetGasStatus()){
                fEventAction->SetPrePos(entryPos);
                fEventAction->SetPostPos(entryPos);
                
            // If entering gas
            }else{
                fEventAction->SetPrePos(entryPos);
                fEventAction->SetGasStatus(true);
            }
        }
        
        // Store positions and total energy of layer for this beam
        fEventAction->AddTrackedEdep(aStep->GetTotalEnergyDeposit());
    }
    
    // Get point particle enters gas volume
    if(preVol->GetName() != "GasLayerRing" &&
        postVol->GetName() == "GasLayerRing" && isMuon){

        fEventAction->SetActualEntry(postStepPoint->GetPosition());
    
        // Create instance of current particle in ROOT file
        G4ThreeVector fActEntry  = fEventAction->GetActualEntry();
        xyzVector entryPos{fActEntry.x(), fActEntry.y(), fActEntry.z()};
        ROOT::Math::PxPyPzEVector mom(
                    initMomentum.x(),
                    initMomentum.y(),
                    initMomentum.z(),
                    fEventAction->GetInitEnergy()
         );
      
      TrackedParticle tParticle(id, entryPos, mom);
      TrackNTupleSvc::instance().addParticle( id, tParticle );
  }
    
    if(preVol->GetName() == "GasLayerRing" &&
     postVol->GetName() != "GasLayerRing"){
         
      fEventAction->SetPostPos(entryPos);
      
      // Get point particle exits gas volume
      fEventAction->SetActualExit(preStepPoint->GetPosition());
      
  } 
  
  
}