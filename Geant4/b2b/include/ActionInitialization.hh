#ifndef B2ActionInitialization_h
#define B2ActionInitialization_h 1

#include "G4VUserActionInitialization.hh"

namespace B2 {

class ActionInitialization : public G4VUserActionInitialization {
  public:
    ActionInitialization() = default;
    ~ActionInitialization() override = default;

    void BuildForMaster() const override;
    void Build() const override;
};

}  // namespace B2

#endif
