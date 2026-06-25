#ifndef B2bDetectorConstruction_h
#define B2bDetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"

#include "G4Threading.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4Material;
class G4UserLimits;
class G4GlobalMagFieldMessenger;

namespace B2b {

class DetectorMessenger;

class DetectorConstruction : public G4VUserDetectorConstruction {
  public:
    DetectorConstruction();
    ~DetectorConstruction() override;

  public:
    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

    // Set methods
    void SetTargetMaterial(G4String);
    void SetChamberMaterial(G4String);
    void SetMaxStep(G4double);
    void SetCheckOverlaps(G4bool);

  private:
    void DefineMaterials();
    G4VPhysicalVolume* DefineVolumes();

    static G4ThreadLocal G4GlobalMagFieldMessenger* fMagFieldMessenger;

    DetectorMessenger* fMessenger = nullptr;  // messenger
    G4UserLimits* fStepLimit = nullptr; // pointer to user step limits

    G4bool fCheckOverlaps = true; // option to activate checking of volumes overlaps
};

}  // namespace B2b

#endif
