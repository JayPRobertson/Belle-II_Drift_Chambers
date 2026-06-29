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
    
    void AddTrackedDistance(G4double distance) { fTrackedDistance += distance; }
    G4double GetTrackedDistance() const { return fTrackedDistance; }
    
    void SetPredictedEntry(const G4ThreeVector& v) { fPredEntry = v; }
    void SetPredictedExit(const G4ThreeVector& v) { fPredExit  = v;  }

    void SetActualEntry(const G4ThreeVector& v) { fActEntry = v; }
    void SetActualExit(const G4ThreeVector& v) { fActExit  = v; }

    G4ThreeVector GetPredictedEntry() const { return fPredEntry; }
    G4ThreeVector GetPredictedExit()  const { return fPredExit; }

    G4ThreeVector GetActualEntry() const { return fActEntry; }
    G4ThreeVector GetActualExit()  const { return fActExit; }

    void AddTrackedEdep(G4double edep) { fTrackedEdep += edep; }
    void ResetTrackedEdep() {fTrackedEdep = 0.0;}
    G4double GetTrackedEdep() const { return fTrackedEdep; }
    
    void SetCurIndex(G4double index) { curIndex = index; }
    G4int GetCurIndex() const { return curIndex; }
    
    void SetPrePos(G4ThreeVector prePos) { 
        prePosX += std::to_string(prePos.x()) + "|";
        prePosY += std::to_string(prePos.y()) + "|";
        prePosZ += std::to_string(prePos.z()) + "|";
    }
    std::string GetPrePos() { 
        return prePosX + "," + prePosY + "," + prePosZ + ","; 
    }
    
    void SetPostPos(G4ThreeVector postPos) { 
        postPosX += std::to_string(postPos.x()) + "|";
        postPosY += std::to_string(postPos.y()) + "|";
        postPosZ += std::to_string(postPos.z()) + "|";
    }
    std::string GetPostPos() { 
        return postPosX + "," + postPosY + "," + postPosZ + ","; 
    }
    
    void SetTotEdep(){
        totEdep += std::to_string(fTrackedEdep) + "|";
    }
    
    std::string GetTotEdep(){ return totEdep; }
    
    bool GetGasStatus() { return enteredGas; }
    void SetGasStatus(bool status) { enteredGas = status; }
    
    void SetInitMomentum(const G4ThreeVector& p) { initMomentum  = p; }
    G4ThreeVector GetInitMomentum()  const { return initMomentum; }
    
    G4int GetParticleID() { return fParticleID; }
    
    void SetInitEnergy(G4double energy) { fInitEnergy = energy;}
    G4double GetInitEnergy() { return fInitEnergy; }
    
    void SetLayerEntryVals(G4ThreeVector pos, G4double time, G4ThreeVector mom){
        fLayerEntry = pos;
        fLayerInitTime = time;
        fLayerInitMomentum = mom;
    }
    
    void AddLayerEdep(G4double edep) { fLayerEdep += edep; }
    G4double GetLayerEdep() { return fLayerEdep;}
    void ResetLayerEdep() { fLayerEdep = 0.0; }
    
    xyzVector GetLayerEntry() { 
        return xyzVector{fLayerEntry.x(), fLayerEntry.y(), fLayerEntry.z()};
    }
    
    G4double GetLayerTime() { return fLayerInitTime; }
    
    xyzVector GetLayerInitMomentum() { 
        return xyzVector{fLayerInitMomentum.x(), fLayerInitMomentum.y(), fLayerInitMomentum.z()};
    }

private:
    G4double fTrackedDistance = 0.0;
    G4double fTrackedEdep = 0.0;    
    G4int curIndex = 0;
    G4int fParticleID = -1;
    
    std::string prePosX = "";
    std::string prePosY = "";
    std::string prePosZ = "";
    
    std::string postPosX = "";
    std::string postPosY = "";
    std::string postPosZ = "";
    
    std::string totEdep = "";

    G4ThreeVector fPredEntry;
    G4ThreeVector fPredExit;

    G4ThreeVector fActEntry;
    G4ThreeVector fActExit;
    
    G4ThreeVector initMomentum; 
    G4double fInitEnergy = 0.0;
    
    bool enteredGas = false;
    
    G4double fLayerEdep = 0.0;
    G4ThreeVector fLayerEntry;
    G4double fLayerInitTime;
    G4ThreeVector fLayerInitMomentum;   
};

}  // namespace B2

#endif