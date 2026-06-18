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

SteppingAction::SteppingAction(B2::EventAction* eventAction)
 : fEventAction(eventAction){}

void SteppingAction::UserSteppingAction(const G4Step* aStep){
  G4Track* track = aStep->GetTrack();
  
  // Don't record data if particle is a delta ray
  const G4VProcess* creatorProcess = track->GetCreatorProcess();
  if (creatorProcess) {
    G4String procName = creatorProcess->GetProcessName();
    if (procName == "eIoni" || procName == "muIoni") {
      return;
    }
  }

  G4StepPoint* preStepPoint  = aStep->GetPreStepPoint();
  G4StepPoint* postStepPoint = aStep->GetPostStepPoint();

  G4VPhysicalVolume* preVol  = preStepPoint->GetPhysicalVolume();
  G4VPhysicalVolume* postVol = postStepPoint->GetPhysicalVolume();
  
  // Verify particle is inside the gas volume
  if(!preVol || !postVol) return;

   // Add step length to total distance
  if(preVol->GetName() == "CylinderPhys"){
    fEventAction->AddTrackedDistance(aStep->GetStepLength());
  }

  // Get point particle enters gas volume
  if(preVol->GetName() != "CylinderPhys" &&
     postVol->GetName() == "CylinderPhys"){

      fEventAction->SetActualEntry(postStepPoint->GetPosition());
  }

  if(preVol->GetName() == "CylinderPhys" &&
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
      
      std::ofstream stepFile("init_step_data.csv", std::ios_base::app);

      G4ThreeVector pos = preStepPoint->GetPosition();
      G4double dist = fEventAction->GetTrackedDistance();

      stepFile << energy << ","
               << pos.x() << "," << pos.y() << "," << pos.z()
               << "," << dist << "," << dEdx << "," << beta*gamma << "\n";
  
               
      stepFile.close();
      
      //____________ Get Entry and Exit Points to GasMix ____________ 
      
      G4ThreeVector fPredEntry = fEventAction->GetPredictedEntry();
      G4ThreeVector fPredExit  = fEventAction->GetPredictedExit();
      G4ThreeVector fActEntry  = fEventAction->GetActualEntry();
      G4ThreeVector fActExit   = fEventAction->GetActualExit();
      
      G4ThreeVector initMomentum = fEventAction->GetInitMomentum();
      
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
  }  
  
}