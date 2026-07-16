#include "TrackerSD.hh"

#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4Step.hh"
#include "G4AffineTransform.hh"
#include "G4TouchableHandle.hh"

namespace B2 {

TrackerSD::TrackerSD(const G4String& name, const G4String& hitsCollectionName)
  : G4VSensitiveDetector(name){
  collectionName.insert(hitsCollectionName);
}

void TrackerSD::Initialize(G4HCofThisEvent* hce){
  // Create hits collection
  fHitsCollection = new TrackerHitsCollection(SensitiveDetectorName, collectionName[0]);

  // Add this collection in hce
  G4int hcID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
  hce->AddHitsCollection(hcID, fHitsCollection);
}


G4bool TrackerSD::ProcessHits(G4Step* step, G4TouchableHistory*){
  // energy deposit
  G4double edep = step->GetTotalEnergyDeposit();
  if (edep == 0.) return false;
    
  G4StepPoint* preStepPoint = step->GetPreStepPoint();
  
  G4TouchableHandle touchable = preStepPoint->GetTouchableHandle();
  G4int chamberNb = touchable->GetCopyNumber();
  G4AffineTransform transform = touchable->GetHistory()->GetTopTransform();
  G4AffineTransform inverseTransform = transform.Inverse();
  
  // Wire position and direction
  G4ThreeVector localCenter(0, 0, 0);
  G4ThreeVector localDirection(0, 0, 1);

  // Tranformation matrix of current volume
  G4ThreeVector globalWireCenter = inverseTransform.TransformPoint(localCenter);
  G4ThreeVector globalWireDir    = inverseTransform.TransformAxis(localDirection).unit();
  
  // Set drift distance
  G4ThreeVector hitPos = preStepPoint->GetPosition();
  G4double driftRadius = ((hitPos - globalWireCenter).cross(globalWireDir)).mag();
  
  auto newHit = new TrackerHit();
  
  // Populate hit
  G4ThreeVector hitPosition = 0.5 * (
        step->GetPreStepPoint()->GetPosition()
        + step->GetPostStepPoint()->GetPosition()
    );

  newHit->SetPos(hitPosition);
  newHit->SetTrackID(step->GetTrack()->GetTrackID());
  newHit->SetEdep(edep);
  newHit->SetChamberNb(chamberNb);
  newHit->SetGlobalTime(preStepPoint->GetGlobalTime());

  fHitsCollection->insert(newHit);

  return true;
}

void TrackerSD::EndOfEvent(G4HCofThisEvent*){
  if (verboseLevel > 1) {
    std::size_t nofHits = fHitsCollection->entries();
    G4cout << G4endl << "-------->Hits Collection: in this event they are " << nofHits
           << " hits in the tracker chambers: " << G4endl;
    for (std::size_t i = 0; i < nofHits; i++)
      (*fHitsCollection)[i]->Print();
  }
}

}  // namespace B2
