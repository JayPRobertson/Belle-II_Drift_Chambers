#ifndef SteppingAction_h
#define SteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"

class G4Step;

namespace B2 {
class EventAction;
}

class SteppingAction: public G4UserSteppingAction {
public:
    SteppingAction(B2::EventAction* eventAction);
    virtual ~SteppingAction() override = default;
    
    virtual void UserSteppingAction(const G4Step* step) override;

private:
    B2::EventAction* fEventAction;
};

#endif