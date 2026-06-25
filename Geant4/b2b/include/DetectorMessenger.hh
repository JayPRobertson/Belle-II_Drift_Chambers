
#ifndef B2bDetectorMessenger_h
#define B2bDetectorMessenger_h 1

#include "G4UImessenger.hh"
#include "globals.hh"

class G4UIdirectory;
class G4UIcmdWithAString;
class G4UIcmdWithADoubleAndUnit;
class G4UIcommand;

namespace B2b {

class DetectorConstruction;

class DetectorMessenger : public G4UImessenger {
  public:
    DetectorMessenger(DetectorConstruction*);
    ~DetectorMessenger() override;

    void SetNewValue(G4UIcommand*, G4String) override;

  private:
    DetectorConstruction* fDetectorConstruction = nullptr;

    G4UIdirectory* fDirectory = nullptr;
    G4UIdirectory* fDetDirectory = nullptr;

    G4UIcmdWithAString* fTargMatCmd = nullptr;
    G4UIcmdWithAString* fChamMatCmd = nullptr;

    G4UIcmdWithADoubleAndUnit* fStepMaxCmd = nullptr;
};

}  // namespace B2b

#endif
