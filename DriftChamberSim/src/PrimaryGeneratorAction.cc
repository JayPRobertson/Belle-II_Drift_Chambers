#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "HelixApproach.hh"
#include "G4RunManager.hh"

#include <cmath>
#include "Randomize.hh"

#include "EventAction.hh"
#include "DetectorConstruction.hh"
#include "RandomGenerator.hh"

namespace DriftChamberSim {
  
auto ranInstance = RandomGenerator::instance();
  
PrimaryGeneratorAction::PrimaryGeneratorAction(EventAction* eventAction): fEventAction(eventAction){
  
  // Create a particle gun to shoot one particle per action through the volume
  G4int nofParticles = 1;
  fParticleGun = new G4ParticleGun(nofParticles);
  
  const G4RunManager* runManager = G4RunManager::GetRunManager();
  const DriftChamberSim::DetectorConstruction* detectorConstruction = dynamic_cast<const DriftChamberSim::DetectorConstruction*>(runManager->GetUserDetectorConstruction());

  G4ParticleDefinition* particleDefinition = G4ParticleTable::GetParticleTable()->FindParticle(detectorConstruction->GetParticleType());

  // Set particle information in the particle gun
  fParticleGun->SetParticleDefinition(particleDefinition);
  fParticleGun->SetParticleEnergy(detectorConstruction->GetParticleEnergy() * GeV);
  fParticleGun->SetParticlePosition(G4ThreeVector(0,0,0));
} 

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event){
    const G4RunManager* runManager = G4RunManager::GetRunManager();
    const DriftChamberSim::DetectorConstruction* detectorConstruction = dynamic_cast<const DriftChamberSim::DetectorConstruction*>(runManager->GetUserDetectorConstruction());
  
    // Set seed of the ran generator
    int seed = detectorConstruction->GetSeed();
    if (seed >= 0) ranInstance.setSeed(seed + fPrimaryCount);
    if (!fPrimaryCount) G4cout << "SEED = " << seed << G4endl;
    fPrimaryCount++;
    
    // Generate a track starting at the origin going in a random direction
    G4double cosTheta = 2.0 * ranInstance.fromUniform(0.,1.) - 1.0;
    G4double sinTheta = std::sqrt(1.0 - cosTheta*cosTheta);

    G4double phi = 2.0 * CLHEP::pi * ranInstance.fromUniform(0,1);

    G4double px = sinTheta*std::cos(phi);
    G4double py = sinTheta*std::sin(phi);
    G4double pz = cosTheta;
    
    G4ThreeVector direction(px, py, pz);
    direction = direction.unit();
    
    fParticleGun->SetParticleMomentumDirection(direction);
    fParticleGun->GeneratePrimaryVertex(event);
    
    // Define particle information locally
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition* particle = particleTable->FindParticle(detectorConstruction->GetParticleType());
    G4double particleMass = particle->GetPDGMass() *MeV;
    G4ThreeVector currentPos = fParticleGun->GetParticlePosition();

    G4double energy = fParticleGun->GetParticleEnergy();
    G4double pMag = std::sqrt(energy*energy - particleMass*particleMass);
    G4ThreeVector momentum = pMag*direction;
    
    G4ThreeVector magneticField = detectorConstruction->GetMagneticField();
    
    G4double charge = fParticleGun->GetParticleDefinition()->GetPDGCharge()/CLHEP::eplus;

    // Use particle info to create a predicted particle trajectory
    HelixApproach helix( currentPos, momentum, magneticField, particleMass, charge);
    
    G4double length = detectorConstruction->GetLength(); 
    G4double rInner = detectorConstruction->GetRInner();  
    G4double rOuter = detectorConstruction->GetROuter();
    
    // Get the predicted entry and exits points to the volume
    G4ThreeVector entry;
    G4ThreeVector exit;
    helix.FindGasVolumeCrossings(rInner, rOuter, length/2, entry, exit);
    
    // Define global constants
    fEventAction->SetPredictedEntry(entry);
    fEventAction->SetPredictedExit(exit);
    fEventAction->SetInitMomentum(momentum);
    fEventAction->SetMass(particleMass);
    fEventAction->SetCharge(charge);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction(){
  delete fParticleGun;
}

}  // namespace