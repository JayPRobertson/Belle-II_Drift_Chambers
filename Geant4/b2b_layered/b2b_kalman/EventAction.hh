#ifndef B2EventAction_h
#define B2EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"
#include "G4ThreeVector.hh"
#include "Math/Vector3D.h"

class G4Event;

using xyzVector = ROOT::Math::XYZVector;

namespace B2{

class EventAction : public G4UserEventAction{
  public:
    EventAction() = default;
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event*) override;
    void EndOfEventAction(const G4Event*) override;
    
    void SetPredictedEntry(const G4ThreeVector& v) { fPredEntry = v; }
    void SetPredictedExit(const G4ThreeVector& v) { fPredExit  = v;  }
    void SetActualEntry(const G4ThreeVector& v) { fActEntry = v; }
    void SetActualExit(const G4ThreeVector& v) { fActExit  = v; }

    G4ThreeVector GetPredictedEntry() const { return fPredEntry; }
    G4ThreeVector GetPredictedExit()  const { return fPredExit; }
    G4ThreeVector GetActualEntry() const { return fActEntry; }
    G4ThreeVector GetActualExit()  const { return fActExit; }
    
    void SetInitMomentum(const G4ThreeVector& p) { initMomentum  = p; }
    G4ThreeVector GetInitMomentum()  const { return initMomentum; }
    
    void SetInitEnergy(G4double energy) { fInitEnergy = energy;}
    G4double GetInitEnergy() { return fInitEnergy; }
    
    void SetCurIndex(G4double index) { curIndex = index; }
    G4int GetCurIndex() const { return curIndex; }
    
    G4int GetParticleID() { return fParticleID; }
    
    bool GetGasStatus() { return enteredGas; }
    void SetGasStatus(bool status) { enteredGas = status; }

private:
    G4int curIndex = 0;
    G4int fParticleID = -1;
    
    G4ThreeVector fPredEntry;
    G4ThreeVector fPredExit;

    G4ThreeVector fActEntry;
    G4ThreeVector fActExit;
    
    G4ThreeVector initMomentum;
    G4double fInitEnergy = 0.0;
    
    bool enteredGas = false; 
};

}  // namespace B2

#endif