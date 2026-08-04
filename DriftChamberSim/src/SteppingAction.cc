#include "SteppingAction.hh"

#include "G4RunManager.hh"
#include "G4StepPoint.hh"
#include "G4VPhysicalVolume.hh"
#include "EventAction.hh"
#include "G4ThreeVector.hh"

#include "G4Track.hh"
#include "G4VProcess.hh"

#include "DetectorConstruction.hh"
#include "TrackNTupleSvc.hh"
#include "TrackedParticle.hh"

using xyzVector = ROOT::Math::XYZVector;

SteppingAction::SteppingAction(DriftChamberSim::EventAction* eventAction)
 : fEventAction(eventAction){}

void SteppingAction::UserSteppingAction(const G4Step* aStep) {
    
    //_________ Check if particle is delta ray _________
    
    const G4RunManager* runManager = G4RunManager::GetRunManager();
    const DriftChamberSim::DetectorConstruction* detectorConstruction = dynamic_cast<const DriftChamberSim::DetectorConstruction*>(runManager->GetUserDetectorConstruction());
    
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

    G4int postIndex = postVol->GetCopyNo();

    // Process only if the particle is currently inside a gas layer
    if (preVol->GetName() == "GasLayerRing") {
        G4int volumeID = preVol->GetCopyNo();
        G4int curIndex = fEventAction->GetCurIndex();

        // Check if new layer entered
        if (curIndex < volumeID) {
            // Initialize tracking for new layer
            fEventAction->SetCurIndex(volumeID);
    
            // If entering gas
            if (!fEventAction->GetGasStatus()){
                fEventAction->SetGasStatus(true);
            }
        }
    }
    
    // Get point particle enters gas volume
    if (preVol->GetName() != "GasLayerRing" &&
        postVol->GetName() == "GasLayerRing"){

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
         
        G4double mass = fEventAction->GetMass();
         
        DriftChamberSim::TrackedParticle tParticle(id, entryPos, mom, !isMuon, mass);
        DriftChamberSim::TrackNTupleSvc::instance().addParticle( id, tParticle );
      
  }
    
    if(preVol->GetName() == "GasLayerRing" &&
     postVol->GetName() != "GasLayerRing"){
         
      // Get point particle exits gas volume
      fEventAction->SetActualExit(preStepPoint->GetPosition());
      
  } 
  
  
}