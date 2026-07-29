#include "DetectorMessenger.hh"

#include "DetectorConstruction.hh"

#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIdirectory.hh"

namespace DriftChamberSim {

DetectorMessenger::DetectorMessenger(DetectorConstruction* Det) : fDetectorConstruction(Det){
  fDirectory = new G4UIdirectory("/DriftChamberSim/");
  fDirectory->SetGuidance("UI commands specific to this example.");

  fDetDirectory = new G4UIdirectory("/DriftChamberSim/det/");
  fDetDirectory->SetGuidance("Detector construction control");

  fTargMatCmd = new G4UIcmdWithAString("/DriftChamberSim/det/setTargetMaterial", this);
  fTargMatCmd->SetGuidance("Select Material of the Target.");
  fTargMatCmd->SetParameterName("choice", false);
  fTargMatCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fChamMatCmd = new G4UIcmdWithAString("/DriftChamberSim/det/setChamberMaterial", this);
  fChamMatCmd->SetGuidance("Select Material of the Chamber.");
  fChamMatCmd->SetParameterName("choice", false);
  fChamMatCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fStepMaxCmd = new G4UIcmdWithADoubleAndUnit("/DriftChamberSim/det/stepMax", this);
  fStepMaxCmd->SetGuidance("Define a step max");
  fStepMaxCmd->SetParameterName("stepMax", false);
  fStepMaxCmd->SetUnitCategory("Length");
  fStepMaxCmd->AvailableForStates(G4State_Idle);
}

DetectorMessenger::~DetectorMessenger() {
  delete fTargMatCmd;
  delete fChamMatCmd;
  delete fStepMaxCmd;
  delete fDirectory;
  delete fDetDirectory;
}

void DetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue) {
  if (command == fTargMatCmd) {
    fDetectorConstruction->SetTargetMaterial(newValue);
  }

  if (command == fChamMatCmd) {
    fDetectorConstruction->SetChamberMaterial(newValue);
  }

  if (command == fStepMaxCmd) {
    fDetectorConstruction->SetMaxStep(fStepMaxCmd->GetNewDoubleValue(newValue));
  }
}

}  // namespace
