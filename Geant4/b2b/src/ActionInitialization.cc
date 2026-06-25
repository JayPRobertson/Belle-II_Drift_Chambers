#include "ActionInitialization.hh"

#include "EventAction.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "SteppingAction.hh"

namespace B2{

void ActionInitialization::BuildForMaster() const{
  SetUserAction(new RunAction);
}


void ActionInitialization::Build() const{
    auto eventAction = new EventAction;
    SetUserAction(eventAction);
    SetUserAction(new RunAction);
    SetUserAction(new PrimaryGeneratorAction(eventAction));
    SetUserAction(new SteppingAction(eventAction));
}

}  // namespace B2