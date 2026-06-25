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

#include "TrackSegment.hh"
#include "TrackNTupleSvc.hh"
#include "TrackedParticle.hh"

using xyzVector = ROOT::Math::XYZVector;

SteppingAction::SteppingAction(B2::EventAction* eventAction)
 : fEventAction(eventAction){}

void SteppingAction::UserSteppingAction(const G4Step* aStep){
  G4Track* track = aStep->GetTrack();
  G4int id = fEventAction->GetParticleID();
  
  bool isMuon = true;
  
  // Don't record data if particle is a delta ray
  const G4VProcess* creatorProcess = track->GetCreatorProcess();
  if (creatorProcess) {
    G4String procName = creatorProcess->GetProcessName();
    if (procName == "eIoni" || procName == "muIoni") {
      isMuon = false;
    }
  }

  G4StepPoint* preStepPoint  = aStep->GetPreStepPoint();
  G4StepPoint* postStepPoint = aStep->GetPostStepPoint();

  G4VPhysicalVolume* preVol  = preStepPoint->GetPhysicalVolume();
  G4VPhysicalVolume* postVol = postStepPoint->GetPhysicalVolume();
  
  G4ThreeVector initMomentum = fEventAction->GetInitMomentum();
  
  // Verify particle is inside the gas volume
  if(!preVol || !postVol) return;

   // Add step length to total distance
  if(preVol->GetName() == "CylinderPhys" && isMuon){
    fEventAction->AddTrackedDistance(aStep->GetStepLength());
  }

  // Get point particle enters gas volume
  if(preVol->GetName() != "CylinderPhys" &&
     postVol->GetName() == "CylinderPhys" && isMuon){

      fEventAction->SetActualEntry(postStepPoint->GetPosition());
      fEventAction->SetEntryTime(track->GetLocalTime());
    
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
  
  // Get point particle leaves gas volume
  else if(preVol->GetName() == "CylinderPhys" &&
     postVol->GetName() != "CylinderPhys"){
      
      //_______________ Calculate Mean Energy Loss __________________
      
      G4double energy = preStepPoint->GetKineticEnergy();
      G4Material* material = preStepPoint->GetMaterial();
      G4ParticleDefinition* particleDef = G4ParticleTable::GetParticleTable()->FindParticle("mu-");
      
      G4double beta = preStepPoint->GetBeta();
      G4double restMass = track->GetDefinition()->GetPDGMass();
      G4double gamma = 1.0 + (energy / restMass);
      
      G4EmCalculator emCalculator;
      G4double dEdx = emCalculator.ComputeElectronicDEDX(energy, particleDef, material);
      
      // Get point particle exits gas volume
      fEventAction->SetActualExit(preStepPoint->GetPosition());
      
      /// Uncomment to write out data to csv file
      /*
      std::ofstream stepFile("init_step_data.csv", std::ios_base::app);

      G4ThreeVector pos = preStepPoint->GetPosition();
      G4double dist = fEventAction->GetTrackedDistance();

      stepFile << energy << ","
               << pos.x() << "," << pos.y() << "," << pos.z()
               << "," << dist << "," << dEdx << "," << beta*gamma << "\n";
  
               
      stepFile.close();
      */
      
      //____________ Get Entry and Exit Points to GasMix ____________ 
      
      if (!isMuon) return;
      
      G4ThreeVector fPredEntry = fEventAction->GetPredictedEntry();
      G4ThreeVector fPredExit  = fEventAction->GetPredictedExit();
      G4ThreeVector fActEntry  = fEventAction->GetActualEntry();
      G4ThreeVector fActExit   = fEventAction->GetActualExit();
      
      G4ThreeVector curMomentum = track->GetMomentum();
      G4double tEntry = fEventAction->GetEntryTime();
      
      // Create instance of current track in ROOT file
      TrackSegment segment(
              xyzVector{fActEntry.x(), fActEntry.y(), fActEntry.z()},
              xyzVector{initMomentum.x(), initMomentum.y(), initMomentum.z()},
              xyzVector{fActExit.x(), fActExit.y(), fActExit.z()},
              xyzVector{curMomentum.x(), curMomentum.y(), curMomentum.z()}, 
              tEntry, dEdx
              );
              
      TrackNTupleSvc::instance().addTrackSegment(id, segment);
      
      /// Uncomment to write out data to csv file
      /*
      std::ofstream eePosFile("entry_exit_data.csv", std::ios_base::app);
      
      eePosFile << initMomentum.x() << "," << initMomentum.y() << ","
                << initMomentum.z() << ","
      
                << fActEntry.x() << "," << fActEntry.y() << ","
                << fActEntry.z() << ","
                 
                << fActExit.x() << "," << fActExit.y() << ","
                << fActExit.z() << ","
                 
                << fPredEntry.x() << "," << fPredEntry.y() << ","
                << fPredEntry.z() << ","
                 
                << fPredExit.x() << "," << fPredExit.y() << ","
                << fPredExit.z() << "\n";
                 
      eePosFile.close();
      */
      
  }  
  
}