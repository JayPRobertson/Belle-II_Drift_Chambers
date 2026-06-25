#include "TrackerHit.hh"

#include "G4Circle.hh"
#include "G4Colour.hh"
#include "G4UnitsTable.hh"
#include "G4VVisManager.hh"
#include "G4VisAttributes.hh"

#include <iomanip>

namespace B2 {

G4ThreadLocal G4Allocator<TrackerHit>* TrackerHitAllocator = nullptr;

G4bool TrackerHit::operator==(const TrackerHit& right) const {
  return (this == &right) ? true : false;
}

void TrackerHit::Draw()
{
  G4VVisManager* pVVisManager = G4VVisManager::GetConcreteInstance();
  if (pVVisManager) {
    G4Circle circle(fPos);
    circle.SetScreenSize(4.);
    circle.SetFillStyle(G4Circle::filled);
    G4VisAttributes attribs(G4Colour::Red());
    circle.SetVisAttributes(attribs);
    pVVisManager->Draw(circle);
  }
}

void TrackerHit::Print(){
  G4cout << "  trackID: " << fTrackID << " chamberNb: " << fChamberNb << "Edep: " << std::setw(7)
         << G4BestUnit(fEdep, "Energy") << " Position: " << std::setw(7)
         << G4BestUnit(fPos, "Length") << G4endl;
}


}  // namespace B2
