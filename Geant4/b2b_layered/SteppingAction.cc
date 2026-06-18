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

#include "DetectorConstruction.hh"

SteppingAction::SteppingAction(B2::EventAction* eventAction)
 : fEventAction(eventAction){}

void SteppingAction::UserSteppingAction(const G4Step* aStep) {
    G4StepPoint* preStepPoint = aStep->GetPreStepPoint();
    G4StepPoint* postStepPoint = aStep->GetPostStepPoint();
    
    G4VPhysicalVolume* preVol = preStepPoint->GetPhysicalVolume();
    G4VPhysicalVolume* postVol = postStepPoint->GetPhysicalVolume();
    
    G4Track* track = aStep->GetTrack();
    G4ThreeVector entryPos = preStepPoint->GetPosition();
    
    const G4RunManager* runManager = G4RunManager::GetRunManager();
    const B2b::DetectorConstruction* detectorConstruction = dynamic_cast<const B2b::DetectorConstruction*>(runManager->GetUserDetectorConstruction());
  
    // Don't record data if particle is a delta ray
    const G4VProcess* creatorProcess = track->GetCreatorProcess();
    if (creatorProcess) {
      G4String procName = creatorProcess->GetProcessName();
      if (procName.substr(procName.length() - 4) == "Ioni") return;
    }

    // Verify particle is inside the volume
    if (!preVol || !postVol) return;

    // Process only if the particle is currently inside a gas layer
    if (preVol->GetName() == "GasLayerRing") {
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
        postVol->GetName() == "GasLayerRing"){

        fEventAction->SetActualEntry(postStepPoint->GetPosition());
    }
    
    if(preVol->GetName() == "GasLayerRing" &&
     postVol->GetName() != "GasLayerRing"){
         
      fEventAction->SetPostPos(entryPos);
      
      //_______________ Calculate Mean Energy Loss __________________
      
      G4double energy = preStepPoint->GetKineticEnergy();
      G4Material* material = preStepPoint->GetMaterial();
      G4ParticleDefinition* particleDef = G4ParticleTable::GetParticleTable()->FindParticle(detectorConstruction->GetParticleType());
      
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