#include "DetectorMessenger.hh"

#include "DetectorConstruction.hh"

#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIdirectory.hh"

namespace DriftChamberSim {

// Add extra UI commands to change simulation variables
DetectorMessenger::DetectorMessenger(DetectorConstruction* Det) : fDetectorConstruction(Det){
  fDirectory = new G4UIdirectory("/DriftChamberSim/");
  fDirectory->SetGuidance("UI commands specific to this example.");

  fDetDirectory = new G4UIdirectory("/DriftChamberSim/det/");
  fDetDirectory->SetGuidance("Detector construction control");

  // Allow step size to be changed
  fStepMaxCmd = new G4UIcmdWithADoubleAndUnit("/DriftChamberSim/det/stepMax", this);
  fStepMaxCmd->SetGuidance("Define a step max");
  fStepMaxCmd->SetParameterName("stepMax", false);
  fStepMaxCmd->SetUnitCategory("Length");
  fStepMaxCmd->AvailableForStates(G4State_Idle);
}

// Free up heap memory
DetectorMessenger::~DetectorMessenger() {
  delete fStepMaxCmd;
  delete fDirectory;
  delete fDetDirectory;
}

// Assign new value if changed by UI command
void DetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue) {
  if (command == fStepMaxCmd) {
    fDetectorConstruction->SetMaxStep(fStepMaxCmd->GetNewDoubleValue(newValue));
  }
}

}  // namespace
