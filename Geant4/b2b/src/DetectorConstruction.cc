#include "DetectorConstruction.hh"

#include "G4ThreeVector.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Colour.hh"
#include "G4Material.hh"
#include "G4SubtractionSolid.hh"
#include "G4UnionSolid.hh"

#include "G4NistManager.hh"
#include "G4GeometryManager.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4VisAttributes.hh"

#include "G4Cons.hh"
#include "G4SystemOfUnits.hh"

#include "G4GlobalMagFieldMessenger.hh"
#include "G4SDManager.hh"
#include "G4AutoDelete.hh"
#include "TrackerSD.hh"
#include "G4UserLimits.hh"

#include <string>

namespace B2b{
  
G4LogicalVolume* cylLog;

G4VPhysicalVolume* DetectorConstruction::Construct(){
  // ____________________ Define Gas Mixtures _____________________ //
  
  G4NistManager* nist = G4NistManager::Instance();
  
  // Define known gases
  G4Element* elH = nist->FindOrBuildElement("H");
  G4Element* elC = nist->FindOrBuildElement("C");
  G4Material* elHe = nist->FindOrBuildMaterial("G4_He");
  G4Material* air = nist->FindOrBuildMaterial("G4_AIR");
  G4Material* al = nist->FindOrBuildMaterial("G4_Al");
  
  // Define ethane (C2H6) gas
  G4double densityEthane = 1.356 * mg/cm3; 
  G4Material* ethane = new G4Material("EthaneGas", densityEthane, 2);
  ethane->AddElement(elC, 2);
  ethane->AddElement(elH, 6);
  
  // Create new He-C2H6 (50/50) gas mixture
  G4double percentHe = 0.50;
  G4double percentEthane = 0.50;
  G4double densityMix = (percentHe * elHe->GetDensity()) + (percentEthane * ethane->GetDensity());
  
  G4Material* gasMix = new G4Material("HeEthaneMix", densityMix, 2);
  gasMix->AddMaterial(elHe, percentHe);
  gasMix->AddMaterial(ethane, percentEthane);

  // Define dimensional constants
  G4double rCenter = 0.0 * cm;
  G4double rOuter = 109.6 * cm;
  G4double rInner = 16. * cm; 
  G4double length = 241.69 * cm;
  G4double inLength = 70.9 * cm;
  
  G4double startAngle = 0.0 * deg;
  G4double spanAngle = 360.0 * deg;
  G4double worldLength = 1.5 * length;
  
  G4double rCone = 48.48*cm;     // Outer radius at bottom
  G4double pDz = 79.29*cm;      // Half height
  G4double pDzSmall = 11.65*cm;
  
  const G4double z1 = 14.579*cm;
  const G4double z2 = 107.145*cm;
  const G4double z3 = length/2;
  
  // _____________________ Define World Size _______________________ //
  
  G4GeometryManager::GetInstance()->SetWorldMaximumExtent(worldLength);

  G4Box* worldS = new G4Box("world", worldLength/2, worldLength/2, worldLength/2);
  G4LogicalVolume* worldLV = new G4LogicalVolume(worldS, air, "World");

  auto worldPV = new G4PVPlacement(nullptr,          // no rotation
                                   G4ThreeVector(),  // at (0,0,0)
                                   worldLV,          // its logical volume
                                   "World",          // its name
                                   nullptr,          // its mother  volume
                                   false,            // no bool operations
                                   0,                // copy number
                                   true);  // checking overlaps
  
  // ___________________ Define Chamber Size ____________________ //
  
  // Create cone objects
  G4Cons* solidCone = new G4Cons("ConeSolid", 0.*cm, rCone, 0.*cm, 0.*cm, pDz, startAngle, spanAngle); 
  G4Cons* smallCone = new G4Cons("smallCone", 0.*cm, rOuter, 0.*cm, 0.*cm, pDzSmall, startAngle, spanAngle);

  // Create cylinder objects
  G4Tubs* outerCyl = new G4Tubs("OuterCyl", rCenter, rOuter, z3, startAngle, spanAngle);
  G4Tubs* innerCyl = new G4Tubs("InnerCyl", rCenter, rInner, (length+10)/2, startAngle, spanAngle);
  
  // Subtract inner cylinder from outer
  G4SubtractionSolid* cylindersS = new G4SubtractionSolid("cylindersS", outerCyl, innerCyl, nullptr, G4ThreeVector(0, 0, 0));
  
  // Subtract cones from cylinders
  G4SubtractionSolid* gasCylS_min1 = new G4SubtractionSolid("gasCylS_min1", cylindersS, solidCone, 0, G4ThreeVector(0, 0, -(z3-pDz)));
  G4SubtractionSolid* gasCylS_min2 = new G4SubtractionSolid("gasCylS_min2", gasCylS_min1, smallCone, 0, G4ThreeVector(0, 0, -(z3-pDzSmall)));
  
  G4RotationMatrix* coneRotation = new G4RotationMatrix();
  coneRotation->rotateY(180.0 * deg);
  
  G4SubtractionSolid* gasCylS_min3 = new G4SubtractionSolid("gasCylS_min3", gasCylS_min2, solidCone, coneRotation, G4ThreeVector(0, 0, (z3-pDz)));
  G4SubtractionSolid* gasCylS = new G4SubtractionSolid("gasCylS", gasCylS_min3, smallCone, coneRotation, G4ThreeVector(0, 0, (z3-pDzSmall)));
  
  // Place cut cylinder filled with gas in world
  cylLog = new G4LogicalVolume(gasCylS, gasMix,"CylinderLog");
  G4VPhysicalVolume* cylPhys = new G4PVPlacement(0, G4ThreeVector(0,0,0), cylLog,"CylinderPhys", worldLV, false, 0, true);
  
  // Create aluminum shell objects
  G4Tubs* outShellS = new G4Tubs("outShellS", rOuter, rOuter + 1.0*cm, z3, startAngle, spanAngle);
  G4Tubs* inShellS = new G4Tubs("inShellS", rInner-1.0*cm, rInner, z1, startAngle, spanAngle);
  
  // Place volume with material in world
  G4LogicalVolume* shellCylLog = new G4LogicalVolume(outShellS, al, "shellCylLog");
  G4VPhysicalVolume* shellCylPhys = new G4PVPlacement(0, G4ThreeVector(0,0,0), shellCylLog,"shellCylPhys", worldLV, false, 0, true);
  
  G4LogicalVolume* inShellLog = new G4LogicalVolume(inShellS, al, "inShellLog");
  G4VPhysicalVolume* inShellPhys = new G4PVPlacement(0, G4ThreeVector(0,0,0), inShellLog,"inShellPhys", worldLV, false, 0, true);
  
  G4VisAttributes* shellVisAtt = new G4VisAttributes(G4Colour::Blue());
  
  // Create aluminum endplates 
  for (int i = 0; i < 77; i++) {
    G4double r1 = 43.8*cm + i*0.85*cm;
    G4double r2 = r1 + 0.85*cm;
    G4double thickness = 1.0*cm;
    
    G4Tubs* ringSolid = new G4Tubs("RingSolid", r1, r2, thickness/2, startAngle, spanAngle);
    G4LogicalVolume* ringLog = new G4LogicalVolume(ringSolid, al, "RingLog");
    
    ringLog->SetVisAttributes(shellVisAtt);
    
    G4double zPosPositive =  107.47*cm + i*0.18*cm; 

    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zPosPositive), ringLog, "EndplateRing_Pos", worldLV, false, i, false);
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, -zPosPositive), ringLog, "EndplateRing_Neg", worldLV, false, i, false);
  }
  
  // ___________________ Print constants ____________________ //
  
  G4double radLength = gasMix->GetRadlen();
  G4cout << " Radiation length: " << radLength *0.1 << " cm" << G4endl;
  
  G4double electronDensity = gasMix->GetElectronDensity();
  G4cout << " Electron density : " << electronDensity << " cm^-3" << G4endl;
  
  G4cout << "GasMix density: " << densityMix << " mg / cm3" << G4endl;
  
  // ___________________ Visualization ____________________ //
  
  G4VisAttributes* gasVisAtt = new G4VisAttributes(G4Colour::White());
  gasVisAtt->SetVisibility(true);
  cylLog->SetVisAttributes(gasVisAtt);
  
  shellVisAtt->SetVisibility(true);
  shellCylLog->SetVisAttributes(shellVisAtt);
  inShellLog->SetVisAttributes(shellVisAtt);
  
  G4VisAttributes* worldVisAtts = new G4VisAttributes(G4Color(1.0, 1.0, 1.0, 0.2)); 
  worldVisAtts->SetVisibility(false);
  worldLV->SetVisAttributes(worldVisAtts);
  
  // Set step size
  G4double maxStep = 0.1 * mm;
  fStepLimit = new G4UserLimits(maxStep);
  cylLog->SetUserLimits(fStepLimit);

  return worldPV;
}

void DetectorConstruction::SetMaxStep(G4double maxStep){
  if ((fStepLimit) && (maxStep > 0.)) fStepLimit->SetMaxAllowedStep(maxStep);
}

void DetectorConstruction::SetTargetMaterial(G4String materialName){}

void DetectorConstruction::SetChamberMaterial(G4String materialName){}

DetectorConstruction::DetectorConstruction(){}

DetectorConstruction::~DetectorConstruction(){ delete fStepLimit; }

void DetectorConstruction::ConstructSDandField(){
 
  // Sensitive detectors
  auto trackerSD = new B2::TrackerSD("B2/gasSD", "TrackerHitsCollection");
  G4SDManager::GetSDMpointer()->AddNewDetector(trackerSD);
  SetSensitiveDetector(cylLog, trackerSD);

  // Create constant magentic field
  G4ThreeVector fieldValue = G4ThreeVector(0., 0., 1.5*tesla);
  auto fMagFieldMessenger = new G4GlobalMagFieldMessenger(fieldValue);
  fMagFieldMessenger->SetVerboseLevel(1);
  
  // Register the field messenger for deleting
  G4AutoDelete::Register(fMagFieldMessenger);
}

}  // namespace B2b
