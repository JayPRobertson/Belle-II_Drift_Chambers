#include "RunAction.hh"

#include "G4RunManager.hh" 

#include "TrackNTupleSvc.hh"
#include "TrackedParticle.hh"

#include "TFile.h"
#include "TTree.h" 

#include "G4ios.hh"

namespace DriftChamberSim {

RunAction::RunAction(){
  G4RunManager::GetRunManager()->SetPrintProgress(1000);
}

void RunAction::BeginOfRunAction(const G4Run*){
  G4RunManager::GetRunManager()->SetRandomNumberStore(false);
  TrackNTupleSvc::instance().fileOpen("particle_and_track_data.root");
}

// Local helper function to print particle data
void readNTuple( const std::string fileName = "particle_and_track_data.root" ){ 
    TFile* input_file = TFile::Open( fileName.c_str() ); 
    TTree* input_tree = (TTree*) input_file->Get("Particles");

    TrackedParticle* particle = new TrackedParticle;
    input_tree->SetBranchAddress("Particle",&particle);

    for ( Long64_t i = 0; i < input_tree->GetEntries(); i++ ){
        input_tree->GetEntry( i );     
        G4cout << "Particle #" << i << ":\n"
                  << "======== \n" 
                  << "            Energy = " << particle->getEnergy() << "\n"
                  << "Summed energy loss = " << particle->getEnergyLoss() 
                  << G4endl; 

        auto segments = particle->getTrackSegments(); 

        for ( auto segment: segments ){
            G4cout << "\tSegment:\n"
                      << "\t======= \n"
                      << "\t      Entry time = " << segment.getEntryTime()  << "\n" 
                      << "\t     Energy loss = " << segment.getEnergyLoss() << "\n"  
                      << "\t Radial position = " << segment.getEntryPosition().Rho() 
                      << G4endl;
        }
        G4cout << G4endl;
    }

    input_file->Close();

    delete particle; 

    return; 
}

void RunAction::EndOfRunAction(const G4Run*) {
  TrackNTupleSvc::instance().fillTree(); 
  TrackNTupleSvc::instance().fileClose();
  //readNTuple();
}


}  // namespace
