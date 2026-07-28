#ifndef B2TrackerHit_h
#define B2TrackerHit_h 1

#include "G4VHit.hh"

#include "G4Allocator.hh"
#include "G4THitsCollection.hh"
#include "G4Threading.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

namespace B2 {

class TrackerHit : public G4VHit {
  public:
    TrackerHit() = default;
    TrackerHit(const TrackerHit&) = default;
    ~TrackerHit() override = default;

    // operators
    TrackerHit& operator=(const TrackerHit&) = default;
    G4bool operator==(const TrackerHit&) const;

    inline void* operator new(size_t);
    inline void operator delete(void*);

    // methods from base class
    void Draw() override;
    void Print() override;

    // Set methods
    void SetTrackID(G4int track) { fTrackID = track; };
    void SetChamberNb(G4int chamb) { fChamberNb = chamb; };
    void SetEdep(G4double de) { fEdep = de; };
    void SetPos(G4ThreeVector xyz) { fPos = xyz; };
    void SetGlobalTime(G4double time) { globalTime = time; }
    void SetMomentum(G4ThreeVector mom) { fMomentum = mom; }

    // Get methods
    G4int GetTrackID() const { return fTrackID; };
    G4int GetChamberNb() const { return fChamberNb; };
    G4double GetEdep() const { return fEdep; };
    G4ThreeVector GetPos() const { return fPos; };
    G4double GetGlobalTime() const { return globalTime; };
    G4ThreeVector GetMomentum() const { return fMomentum; };

  private:
    G4int fTrackID = -1;       // Manual track ID tracker
    G4double fEdep = 0.;       // Energy deposition 
    G4ThreeVector fPos;        // Global position of the step
    G4int fChamberNb = -1;     // Wire ID
    G4ThreeVector fMomentum;    // Step momentum
    
    G4double globalTime = 0.;
    
};

using TrackerHitsCollection = G4THitsCollection<TrackerHit>;

extern G4ThreadLocal G4Allocator<TrackerHit>* TrackerHitAllocator;

inline void* TrackerHit::operator new(size_t) {
  if (!TrackerHitAllocator) TrackerHitAllocator = new G4Allocator<TrackerHit>;
  return (void*)TrackerHitAllocator->MallocSingle();
}

inline void TrackerHit::operator delete(void* hit) {
  TrackerHitAllocator->FreeSingle((TrackerHit*)hit);
}

}  // namespace B2

#endif

