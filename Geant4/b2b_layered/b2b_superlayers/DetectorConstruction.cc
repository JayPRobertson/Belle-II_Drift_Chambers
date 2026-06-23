#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Colour.hh"
#include "G4Material.hh"

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

#include <iostream>
#include <cmath>
#include <vector>
#include <fstream>

namespace B2b {

void DetectorConstruction::ReadGeometryFile() {
    std::ifstream geomFile("geometry.json");

    if (!geomFile.is_open()) {
        std::cerr << "Error: Could not open geometry.json" << std::endl;
        return;
    }

    try {
        jsonData = json::parse(geomFile);
        geomFile.close();
    }
    catch (const json::parse_error& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
    }
}

G4Material* DetectorConstruction::CreateMaterialFromJson(G4NistManager* nist, json mixture){
  std::vector<G4Material*> materialList;
  G4double densityMix = 0;
  std::string mixName = "";
  
  // Create materials from element equations
  for (const auto& [materialName, materialData] : mixture.items()) {
    json equation = materialData["equation"];
    
    mixName += materialName;
    
    G4double elementDensity = materialData["density_mg/cm3"].get<G4double>();
    G4double gasPercent = materialData["percent"].get<G4double>();
    
    G4Material* curMaterial = new G4Material(materialName, 
                                             elementDensity * mg/cm3, 
                                             static_cast<G4int>(equation.size())
                                            );
    
    for (const auto& [elementName, num] : equation.items()){
        G4Element* curElement = nist->FindOrBuildElement(elementName);
        curMaterial->AddElement(curElement, num.get<G4int>());
    }
   
    materialList.push_back(curMaterial);
    densityMix += gasPercent * elementDensity * mg/cm3;
  }
  
  G4int n = static_cast<G4int>(materialList.size());
  G4Material* materialMix = new G4Material(mixName + "Mix", densityMix, n);
  
  // Create gas mixture
  for (int i = 0; i < n; i++){
    G4Material* curMaterial = materialList[i];
    G4double percent = mixture[curMaterial->GetName()]["percent"].get<G4double>();
    materialMix->AddMaterial(curMaterial, percent);
  }
  
  return materialMix;
}

void DetectorConstruction::DefineMaterials(){
  G4NistManager* nist = G4NistManager::Instance();
  
  fWorldMaterial = nist->FindOrBuildMaterial("G4_AIR");
  
  json mixture = jsonData["gas_volume"]["mixture"];
  fGasMaterial = CreateMaterialFromJson(nist, mixture);
  
  mixture = jsonData["shell_volume"]["mixture"];
  G4Material* shellMix = CreateMaterialFromJson(nist, mixture);
  fShellMaterial = shellMix;
  
  mixture = jsonData["endplates_volume"]["mixture"];
  if (mixture["same_as_shell"].get<bool>()){
    fEndplateMaterial = shellMix;
  } else {
    fEndplateMaterial = CreateMaterialFromJson(nist, mixture);
  }
  
  mixture = jsonData["layers"]["wire_info"]["sense_wires"]["mixture"];
  fSenseWireMaterial = CreateMaterialFromJson(nist, mixture);
  
  mixture = jsonData["layers"]["wire_info"]["ground_wires"]["mixture"];
  fGroundWireMaterial = CreateMaterialFromJson(nist, mixture);
  
  
}

G4VPhysicalVolume* DetectorConstruction::Construct(){
  ReadGeometryFile();
  DefineMaterials();
  G4NistManager* nist = G4NistManager::Instance();
  
  // _____________________ Define Constants _______________________ //
  
  // Define dimensional constants
  G4double rCenter = 0.0 *cm;
  G4double startAngle = 0.0 *deg;
  G4double spanAngle = 360.0 *deg;
  
  // Gas volume
  G4int numSuperlayers = jsonData["layers"]["num_of_superlayers"].get<G4int>();
  
  std::vector<G4int> numSublayers;
  std::vector<G4double> outerRadii;
  std::vector<G4int> numSenseWires;
  json dimensions = jsonData["super_layers"];
  
  for (int i=0; i < numSuperlayers; i++){
    json curLayer = dimensions["layer"+std::to_string(i)];
    G4int curSublayers = curLayer["num_sublayers"].get<G4int>();
    G4double curOuterRadius = curLayer["outer_radius_cm"].get<G4double>() *cm;
    G4int curSenseWires = curLayer["num_sublayer_sense_wires"].get<G4int>();
    
    numSublayers.push_back(curSublayers);
    outerRadii.push_back(curOuterRadius);
    numSenseWires.push_back(curSenseWires);
  }
  
  dimensions = jsonData["gas_volume"]["dimensions"];
  G4double length = dimensions["full_length_cm"].get<G4double>() *cm;
  G4double rInner = dimensions["radius_inner_cm"].get<G4double>() *cm;
  G4double rOuter = outerRadii.back(); 
                              
  G4double worldLength = 1.5 * length;
  
  // Cone cutouts
  G4double longAngle = dimensions["long_cone_cutout"]["angle_deg"].get<G4double>() * deg;
  G4double shortAngle = dimensions["short_cone_cutout"]["angle_deg"].get<G4double>() * deg;
  G4double longR = dimensions["long_cone_cutout"]["radius_cm"].get<G4double>() * cm;
  G4double shortR = dimensions["short_cone_cutout"]["radius_cm"].get<G4double>() * cm;
  G4double pDzLong = (longR / std::tan(longAngle)) / 2;
  G4double pDzShort = (shortR / std::tan(shortAngle)) / 2;
  
  // Shell volume
  dimensions = jsonData["shell_volume"]["dimensions"];
  G4double thicknessShell = dimensions["thickness_cm"].get<G4double>() * cm;
  
  // Endplates volume
  dimensions = jsonData["endplates_volume"]["dimensions"];
  G4double thicknessEndplate = dimensions["thickness_cm"].get<G4double>() * cm;
  
  // Wire volume
  dimensions = jsonData["layers"]["wire_info"];
  G4double senseRadius = dimensions["sense_wires"]["radius_mm"].get<G4double>() *mm;
  G4double groundRadius = dimensions["ground_wires"]["radius_mm"].get<G4double>() *mm;
  G4double senseVolume = CLHEP::pi * senseRadius*senseRadius * length;
  G4double groundVolume = CLHEP::pi * senseRadius*senseRadius * length;
  
  bool useWireMaterials = dimensions["use_wire_materials"].get<bool>();

  // _____________________ Define World Size _______________________ //
  
  G4GeometryManager::GetInstance()->SetWorldMaximumExtent(worldLength);

  G4Box* worldS = new G4Box("world", worldLength/2, worldLength/2, worldLength/2);
  G4LogicalVolume* worldLV = new G4LogicalVolume(worldS, fWorldMaterial, "World");

  auto worldPV = new G4PVPlacement(nullptr,          // no rotation
                                   G4ThreeVector(),  // at (0,0,0)
                                   worldLV,          // its logical volume
                                   "World",          // its name
                                   nullptr,          // its mother volume
                                   false,            // no bool operations
                                   0,                // copy number
                                   true);            // checking overlaps
  
  // ___________________ Define Chamber Size ____________________ //
  
  G4VisAttributes* shellVisAtt = new G4VisAttributes(G4Colour::Blue());
  G4VisAttributes* gasVisAtt = new G4VisAttributes(G4Colour::White());
  
  auto trackerSD = new B2::TrackerSD("B2/gasSD", "TrackerHitsCollection");
  G4SDManager::GetSDMpointer()->AddNewDetector(trackerSD);
  
  // Set step size
  G4double maxStep = 0.1 * mm;
  fStepLimit = new G4UserLimits(maxStep);
  
  std::ofstream layerFile("layer_radius.csv", std::ios_base::app);
  
  // Large number to prevent early termination
  const int maxI = 5000;
  
  // Transition points where cutout angle changes
  const G4double z3 = length/2;
  const G4double z1 = z3 - ((longR-rInner) /std::tan(longAngle));
  const G4double z2 = z3 - ((shortR-rInner) /std::tan(shortAngle));
  
  // Current superlayer information
  int curSuperlayer = 0;
  int curSublayer = 0;
  
  G4double curSpacing = (outerRadii[0]-rInner)/numSublayers[0];
  G4double prevSpacing = curSpacing;
  G4double curThickness = curSpacing/std::tan(longAngle);
  
  bool isSwitched = false;
  G4double r1;
  G4double r2 = rInner;
  G4double thickness = z1;
  
  for (int i = 0; i < maxI; i++) {
    r1 = r2;
    r2 += curSpacing;
    
    // Update at boundary between cone cutouts
    if (!(thickness < z2 && thickness + curThickness < z2) && !isSwitched){
      curThickness = curSpacing/std::tan(shortAngle);
      isSwitched = true;
    }
    
    thickness += curThickness;
    
    curSublayer++;
    G4int curNumSublayers = numSublayers[curSuperlayer];
    
    // End if exceeded num superlayers
    if (curSuperlayer+1 == numSuperlayers
              && curSublayer == curNumSublayers) break;
    
    // Switch superlayers                      
    if (curSublayer == curNumSublayers) {
        curSuperlayer++;
        curSublayer = 0;
      
        curSpacing = (outerRadii[curSuperlayer]-r2)/curNumSublayers;
      
        if (!isSwitched){
          curThickness = curSpacing/std::tan(longAngle);
        }else{
          curThickness = curSpacing/std::tan(shortAngle);
        }
      
    }                       
                            
    layerFile << r1 << "," << r2 << "\n";
    
    G4Tubs* cylRing = new G4Tubs("CylRing", r1, r2, thickness, startAngle, spanAngle);
    G4LogicalVolume* cylRingLog;
    
    if (useWireMaterials){
      // Get number of wires in the sublayer
      G4int senseWiresPerLayer = numSenseWires[curSuperlayer];
      G4int groundWiresPerLayer = 3*senseWiresPerLayer + 2*senseWiresPerLayer;
      G4double totalSenseVolume = senseVolume * senseWiresPerLayer;
      G4double totalGroundVolume = groundVolume * groundWiresPerLayer;
      
      // Calculate the percentage of the volume of each wire type
      G4double cylRingVolume = cylRing->GetCubicVolume();
      G4double sensePercent = totalSenseVolume / cylRingVolume;
      G4double groundPercent = totalGroundVolume / cylRingVolume;
      G4double gasPercent = 1.0 - sensePercent - groundPercent;
      
      // Get the density of each sublayer material
      G4double senseDensity = fSenseWireMaterial->GetDensity();
      G4double groundDensity = fGroundWireMaterial->GetDensity();
      G4double gasDensity = fGasMaterial->GetDensity();
      
      // Create a material combining the gas and wire material proportionally
      G4double densityMix =  sensePercent*senseDensity + groundPercent*groundDensity + gasPercent*gasDensity;
      G4Material* materialMix = new G4Material("GasMix" + std::to_string(i), densityMix, 3);
      materialMix->AddMaterial(fSenseWireMaterial, sensePercent);
      materialMix->AddMaterial(fGroundWireMaterial, groundPercent);
      materialMix->AddMaterial(fGasMaterial, gasPercent);
      
      cylRingLog = new G4LogicalVolume(cylRing, materialMix, "CylRingLog");
    }else{
      cylRingLog = new G4LogicalVolume(cylRing, fGasMaterial, "CylRingLog");
    }
    
    cylRingLog->SetVisAttributes(gasVisAtt);
    cylRingLog->SetUserLimits(fStepLimit);
    
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0), cylRingLog, "GasLayerRing", worldLV, false, i, false);
    
    SetSensitiveDetector(cylRingLog, trackerSD);
    
    if (thickness >= z2){
      G4Tubs* ringSolid = new G4Tubs("RingSolid", r1, r2, thicknessEndplate, startAngle, spanAngle);
      G4LogicalVolume* ringLog = new G4LogicalVolume(ringSolid, fEndplateMaterial, "RingLog");
      
      ringLog->SetVisAttributes(shellVisAtt); 
      
      G4double pos = thickness + curThickness;
      
      new G4PVPlacement(nullptr, G4ThreeVector(0, 0, pos), ringLog, "EndplateRing_Pos", worldLV, false, i, false);
      new G4PVPlacement(nullptr, G4ThreeVector(0, 0, -pos), ringLog, "EndplateRing_Neg", worldLV, false, i, false);
    }
  }
  
  layerFile.close();
  
  // Create aluminum shell objects
  G4Tubs* outShellS = new G4Tubs("outShellS", r1, r1+thicknessShell, thickness, startAngle, spanAngle);
  G4Tubs* inShellS = new G4Tubs("inShellS", rInner-thicknessShell, rInner, z1, startAngle, spanAngle);
  
  // Place volume with material in world
  G4LogicalVolume* shellCylLog = new G4LogicalVolume(outShellS, fShellMaterial, "shellCylLog");
  G4VPhysicalVolume* shellCylPhys = new G4PVPlacement(0, G4ThreeVector(0,0,0), shellCylLog,"shellCylPhys", worldLV, false, 0, true);
  
  G4LogicalVolume* inShellLog = new G4LogicalVolume(inShellS, fShellMaterial, "inShellLog");
  G4VPhysicalVolume* inShellPhys = new G4PVPlacement(0, G4ThreeVector(0,0,0), inShellLog,"inShellPhys", worldLV, false, 0, true);
  
  // ___________________ Print constants ____________________ //
  
  G4double radLength = fGasMaterial->GetRadlen();
  G4cout << " Radiation length: " << radLength *0.1 << " cm" << G4endl;
  
  G4double electronDensity = fGasMaterial->GetElectronDensity();
  G4cout << " Electron density : " << electronDensity << " cm^-3" << G4endl;
  
  // ___________________ Visualization ____________________ //
  
  shellVisAtt->SetVisibility(true);
  
  gasVisAtt->SetVisibility(true);
  shellCylLog->SetVisAttributes(shellVisAtt);
  inShellLog->SetVisAttributes(shellVisAtt);
  
  shellVisAtt->SetForceSolid(true);
  
  G4VisAttributes* worldVisAtts = new G4VisAttributes(G4Color(1.0, 1.0, 1.0, 0.2)); 
  worldVisAtts->SetVisibility(false);
  worldLV->SetVisAttributes(worldVisAtts);
  
  volumeLength = length;
  volumeROuter = rOuter;
  volumeRInner = rInner;

  return worldPV;
}

void DetectorConstruction::SetMaxStep(G4double maxStep){
  if ((fStepLimit) && (maxStep > 0.)) fStepLimit->SetMaxAllowedStep(maxStep);
}

void DetectorConstruction::SetTargetMaterial(G4String materialName){}

void DetectorConstruction::SetChamberMaterial(G4String materialName){}

DetectorConstruction::DetectorConstruction(){
  ReadGeometryFile(); 
  
  // Particle information
  particleType = jsonData["particle"]["particle_type"];
  particleEnergy = jsonData["particle"]["energy_GeV"].get<G4double>();
  ReadGeometryFile(); 
  
  // Particle information
  particleType = jsonData["particle"]["particle_type"];
  particleEnergy = jsonData["particle"]["energy_GeV"].get<G4double>();
  }

DetectorConstruction::~DetectorConstruction(){  delete fStepLimit; }

void DetectorConstruction::ConstructSDandField(){
  
  json bFieldData = jsonData["magnetic_field_teslas"];

  // Create constant magentic field
  G4double x = bFieldData["x"].get<G4double>() * tesla;
  G4double y = bFieldData["y"].get<G4double>() * tesla;
  G4double z = bFieldData["z"].get<G4double>() * tesla;

  magneticField = G4ThreeVector(x, y, z);

                                           
  auto fMagFieldMessenger = new G4GlobalMagFieldMessenger(magneticField);
  fMagFieldMessenger->SetVerboseLevel(1);
  
  // Register the field messenger for deleting
  G4AutoDelete::Register(fMagFieldMessenger);
}

} // namespace B2b
