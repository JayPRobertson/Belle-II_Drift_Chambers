#ifndef B2TrackerSD_h
#define B2TrackerSD_h 1

#include "G4VSensitiveDetector.hh"

#include "TrackerHit.hh"

#include "globals.hh"

class G4Step;
class G4HCofThisEvent;
class G4TouchableHistory;

namespace B2 {

class TrackerSD : public G4VSensitiveDetector {
  public:
    TrackerSD(const G4String& name, const G4String& hitsCollectionName);
    ~TrackerSD() override = default;

    // methods from base class
    void Initialize(G4HCofThisEvent* hitCollection) override;
    G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
    void EndOfEvent(G4HCofThisEvent* hitCollection) override;

  private:
    TrackerHitsCollection* fHitsCollection = nullptr;
};

}  // namespace B2

#endif
