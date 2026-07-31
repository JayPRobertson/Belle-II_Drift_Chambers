#include "ActionInitialization.hh"

#include "EventAction.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"

namespace DriftChamberSim {

// Actions taken by the master thread
void ActionInitialization::BuildForMaster() const{
  SetUserAction(new RunAction);
}

// Actions taken by the worker threads
void ActionInitialization::Build() const{
    auto eventAction = new EventAction;
    SetUserAction(eventAction);
    SetUserAction(new RunAction);
    SetUserAction(new PrimaryGeneratorAction(eventAction));
    SetUserAction(new SteppingAction(eventAction));
}

}  // namespace