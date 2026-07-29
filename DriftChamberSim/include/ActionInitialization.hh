#ifndef DCActionInitialization_h
#define DCActionInitialization_h 1

#include "G4VUserActionInitialization.hh"

namespace DriftChamberSim {

class ActionInitialization : public G4VUserActionInitialization {
  public:
    ActionInitialization() = default;
    ~ActionInitialization() override = default;

    void BuildForMaster() const override;
    void Build() const override;
};

}  //namespace

#endif
