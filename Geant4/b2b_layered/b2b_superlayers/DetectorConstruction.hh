#ifndef B2bDetectorConstruction_h
#define B2bDetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "G4Threading.hh"
#include "G4NistManager.hh"
#include "G4ThreeVector.hh"

#include <string>
#include <nlohmann/json.hpp>
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4Material;
class G4UserLimits;
class G4GlobalMagFieldMessenger;

namespace B2b{

using json = nlohmann::json;

class DetectorMessenger;

class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction();
    ~DetectorConstruction() override;

  public:
    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;
    void ReadGeometryFile();
    G4Material* CreateMaterialFromJson(G4NistManager* nist, json mixture);

    // Set methods
    void SetTargetMaterial(G4String);
    void SetChamberMaterial(G4String);
    void SetMaxStep(G4double);
    void SetCheckOverlaps(G4bool);
    
    G4ThreeVector GetMagneticField() const { return magneticField; }
    G4double GetLength() const { return volumeLength; }
    G4double GetROuter() const { return volumeROuter; }
    G4double GetRInner() const { return volumeRInner; }
    
    std::string GetParticleType() const { return particleType; }
    G4double GetParticleEnergy() const { return particleEnergy; }
    
    void SetSeed(int seed) { fSeed = seed; }
    int GetSeed() const { return fSeed; }

  private:
    void DefineMaterials();
    G4VPhysicalVolume* DefineVolumes();

    static G4ThreadLocal G4GlobalMagFieldMessenger* fMagFieldMessenger;

    G4Material* fWorldMaterial = nullptr;
    G4Material* fGasMaterial = nullptr;
    G4Material* fShellMaterial = nullptr; 
    G4Material* fEndplateMaterial = nullptr;
    G4Material* fSenseWireMaterial = nullptr;
    G4Material* fGroundWireMaterial = nullptr;
    
    json jsonData;
    G4ThreeVector magneticField;

    DetectorMessenger* fMessenger = nullptr; 
    G4UserLimits* fStepLimit = nullptr;

    G4bool fCheckOverlaps = true; 
    
    G4double volumeLength;
    G4double volumeROuter;
    G4double volumeRInner;
    
    std::string particleType = "EMPTY";
    G4double particleEnergy;   //[GeV]
    
    int fSeed = -1;
};

}  // namespace B2b

#endif
