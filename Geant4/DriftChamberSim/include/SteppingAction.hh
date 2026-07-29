#ifndef DCSteppingAction_h
#define DCSteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"

class G4Step;

namespace DriftChamberSim {
    class EventAction;
}

class SteppingAction: public G4UserSteppingAction {
public:
    SteppingAction(DriftChamberSim::EventAction* eventAction);
    virtual ~SteppingAction() override = default;
    
    virtual void UserSteppingAction(const G4Step* step) override;

private:
    DriftChamberSim::EventAction* fEventAction;
};

#endif