#include "EventAction.hh"

#include <iostream>
#include <fstream>
#include <string>

#include "G4ThreeVector.hh"
#include "G4Event.hh"
#include "G4TrajectoryContainer.hh"
#include "globals.hh"
#include "TrackerHit.hh"
#include "G4SDManager.hh"
#include "HelixApproach.hh"


namespace B2{
  
G4int fTrackerHCID = -1;

void EventAction::BeginOfEventAction(const G4Event*) {
  if (fTrackerHCID == -1) {
        fTrackerHCID = G4SDManager::GetSDMpointer()->GetCollectionID("TrackerHitsCollection");
  }

  fTrackedDistance = 0.0;
  fParticleID++;
}

void EventAction::EndOfEventAction(const G4Event* event){
  
  // ___________________ Write out event data _______________________ //
  
  // Open csv file to write out data
  std::ofstream eventsFile("event_action_data.csv", std::ios_base::app);
  
  // Get number of stored trajectories
  G4TrajectoryContainer* trajectoryContainer = event->GetTrajectoryContainer();
  std::size_t n_trajectories = 0;
  if (trajectoryContainer) n_trajectories = trajectoryContainer->entries();

  // Periodic printing
  G4int eventID = event->GetEventID();
  if (eventID < 100 || eventID % 100 == 0) {
    G4cout << ">>> Event: " << eventID << G4endl;
    eventsFile << eventID << ",";
    
    if (trajectoryContainer) {
      G4cout << "    " << n_trajectories << " trajectories stored in this event." << G4endl;
      eventsFile << n_trajectories << ",";
    }else{
      eventsFile << "0,";
    }
    auto hce = event->GetHCofThisEvent();

    if (hce){
      auto hc = hce->GetHC(fTrackerHCID);
    
      if (hc){  
        eventsFile << hc->GetSize() << ",";
        
        std::string hitEdep = "";
        std::string hitX= "";
        std::string hitY = "";
        std::string hitZ = "";
        
        for (size_t i = 0; i < hc->GetSize(); i++) {
            TrackerHit* hit = static_cast<TrackerHit*>(hc->GetHit(i));
            
            hitEdep += std::to_string(hit->GetEdep()) + "|";
            
            G4ThreeVector hitPos = hit->GetPos();
            hitX += std::to_string(hitPos.getX()) + "|";
            hitY += std::to_string(hitPos.getY()) + "|";
            hitZ += std::to_string(hitPos.getZ()) + "|";
        }
        eventsFile << hitEdep << "," \
                   << hitX << "," << hitY << "," << hitZ << "\n";
        
      }else {
        eventsFile << "0,,,\n";
      }
    }else {
      eventsFile << "0,,,\n";
    }
  }
}

}  // namespace B2
