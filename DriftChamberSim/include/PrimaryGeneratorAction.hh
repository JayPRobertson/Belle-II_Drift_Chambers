#ifndef DCPrimaryGeneratorAction_h
#define DCPrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"

class G4ParticleGun;
class G4Event;

namespace DriftChamberSim {

class EventAction;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction{
    public:
        PrimaryGeneratorAction(EventAction* eventAction);
        ~PrimaryGeneratorAction() override;
    
        void GeneratePrimaries(G4Event*) override;
    
        G4ParticleGun* GetParticleGun() { return fParticleGun; }
    
    private:
        G4ParticleGun* fParticleGun = nullptr;
        EventAction* fEventAction = nullptr;  
        int fPrimaryCount = 0;
};

}  // namespace

#endif
