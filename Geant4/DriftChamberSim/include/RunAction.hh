#ifndef DCRunAction_h
#define DCRunAction_h 1

#include "G4UserRunAction.hh"

class G4Run;

namespace DriftChamberSim {

class RunAction : public G4UserRunAction {
  public:
    RunAction();
    ~RunAction() override = default;

    void BeginOfRunAction(const G4Run* run) override;
    void EndOfRunAction(const G4Run* run) override;
};

}  // namespace

#endif
