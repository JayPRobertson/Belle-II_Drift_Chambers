#include "PrimaryGeneratorAction.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "globals.hh"
#include "HelixApproach.hh"
#include "G4RunManager.hh"

#include "Randomize.hh"
#include <cmath>
#include <string>
#include <nlohmann/json.hpp>

#include "EventAction.hh"
#include "DetectorConstruction.hh"

namespace B2{
  
PrimaryGeneratorAction::PrimaryGeneratorAction(EventAction* eventAction): fEventAction(eventAction){
  G4int nofParticles = 1;
  fParticleGun = new G4ParticleGun(nofParticles);
  
  const G4RunManager* runManager = G4RunManager::GetRunManager();
  const B2b::DetectorConstruction* detectorConstruction = dynamic_cast<const B2b::DetectorConstruction*>(runManager->GetUserDetectorConstruction());

  G4ParticleDefinition* particleDefinition = G4ParticleTable::GetParticleTable()->FindParticle(detectorConstruction->GetParticleType());

  fParticleGun->SetParticleDefinition(particleDefinition);
  fParticleGun->SetParticleEnergy(detectorConstruction->GetParticleEnergy() * GeV);
  fParticleGun->SetParticlePosition(G4ThreeVector(0,0,0));
} 

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event){
    const G4RunManager* runManager = G4RunManager::GetRunManager();
    const B2b::DetectorConstruction* detectorConstruction = dynamic_cast<const B2b::DetectorConstruction*>(runManager->GetUserDetectorConstruction());
  
    // Set seed of the ran generator
    int seed = detectorConstruction->GetSeed();
    if (seed >= 0) G4Random::setTheSeed(seed + fPrimaryCount);
    if (!fPrimaryCount) G4cout << "SEED = " << seed << G4endl;
    fPrimaryCount++;

    // Generate a track starting at the origin going in a random direction
    G4double cosTheta = 2.0*G4UniformRand() - 1.0;
    G4double sinTheta = std::sqrt(1.0 - cosTheta*cosTheta);

    G4double phi = 2.0*CLHEP::pi*G4UniformRand();

    G4double px = sinTheta*std::cos(phi);
    G4double py = sinTheta*std::sin(phi);
    G4double pz = cosTheta;

    G4ThreeVector direction(px, py, pz);
    direction = direction.unit();

    fParticleGun->SetParticleMomentumDirection(direction);
    fParticleGun->GeneratePrimaryVertex(event);

    G4ThreeVector entry;
    G4ThreeVector exit;
    
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition* particle = particleTable->FindParticle(detectorConstruction->GetParticleType());
    G4double particleMass = particle->GetPDGMass() *MeV;
    G4ThreeVector currentPos = fParticleGun->GetParticlePosition();

    G4double kineticE = fParticleGun->GetParticleEnergy();
    G4double totalE = kineticE + particleMass;
    G4double pMag = std::sqrt(totalE*totalE - particleMass*particleMass);
    G4ThreeVector momentum = pMag*direction;
    
    G4ThreeVector magneticField = detectorConstruction->GetMagneticField();
    
    G4double charge = fParticleGun->GetParticleDefinition()->GetPDGCharge()/CLHEP::eplus;

    HelixApproach helix( currentPos, momentum, magneticField, particleMass, charge);
    
    G4double rOuter = detectorConstruction->GetLength(); 
    G4double rInner = detectorConstruction->GetRInner();  
    G4double length = detectorConstruction->GetROuter();

    helix.FindGasVolumeCrossings(rInner, rOuter, length/2, entry, exit);
    
    fEventAction->SetPredictedEntry(entry);
    fEventAction->SetPredictedExit(exit);
    
    fEventAction->SetInitMomentum(momentum);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction(){
  delete fParticleGun;
}

}  // namespace B2