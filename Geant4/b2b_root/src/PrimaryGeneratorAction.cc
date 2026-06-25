#include "PrimaryGeneratorAction.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "globals.hh"
#include "HelixApproach.hh"
#include "G4MuonMinus.hh"

#include "Randomize.hh"
#include <cmath>

#include "EventAction.hh"

namespace B2{
  
PrimaryGeneratorAction::PrimaryGeneratorAction(EventAction* eventAction): fEventAction(eventAction){
  G4int nofParticles = 1;
  fParticleGun = new G4ParticleGun(nofParticles);

  G4ParticleDefinition* particleDefinition = G4ParticleTable::GetParticleTable()->FindParticle("mu-");

  fParticleGun->SetParticleDefinition(particleDefinition);
  fParticleGun->SetParticleEnergy(2.0 *GeV);
  fParticleGun->SetParticlePosition(G4ThreeVector(0,0,0));
} 

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event){
    
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

    G4double rOuter = 109.6 *cm; 
    G4double rInner = 16.0 *cm;  
    G4double length = 241.69 *cm;
    
    G4double muMass = G4MuonMinus::Definition()->GetPDGMass();
    G4ThreeVector currentPos = fParticleGun->GetParticlePosition();

    G4double energy = fParticleGun->GetParticleEnergy();
    G4double pMag = std::sqrt(energy*energy - muMass*muMass);
    G4ThreeVector momentum = pMag*direction;
    
    G4double charge = fParticleGun->GetParticleDefinition()->GetPDGCharge()/CLHEP::eplus;

    HelixApproach helix(currentPos, momentum, G4ThreeVector(0,0,1.5*tesla), muMass, charge);

    helix.FindGasVolumeCrossings(rInner, rOuter, length/2, entry, exit);

    fEventAction->SetPredictedEntry(entry);
    fEventAction->SetPredictedExit(exit);
    
    fEventAction->SetInitMomentum(direction);
    fEventAction->SetInitEnergy(energy);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction(){
  delete fParticleGun;
}

}  // namespace B2