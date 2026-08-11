#include "TrackNTupleSvc.hh"
#include <algorithm> 
#include "G4ios.hh"
#include "globals.hh"

namespace DriftChamberSim {

ClassImp(TrackNTupleSvc)

TrackNTupleSvc::TrackNTupleSvc() 
    : m_particlePtr( new TrackedParticle ), 
      m_deltaPtr( new TrackedParticle )
{}

TrackNTupleSvc::~TrackNTupleSvc(){
    delete m_particlePtr;
    delete m_deltaPtr;
}

// Write out data and free memory
void TrackNTupleSvc::fileClose(){
    if (!m_file) return;

    m_file->cd();
    m_tree->Write();

    delete m_tree;
    m_tree = nullptr;

    m_file->Close();
    delete m_file;
    m_file = nullptr;
}

TrackNTupleSvc& TrackNTupleSvc::instance() {
    static TrackNTupleSvc instance; 
    return instance; 
}

// Open a ROOT file and add new branches
void TrackNTupleSvc::fileOpen( const std::string& fileName ){
    m_file = new TFile( fileName.c_str(), "RECREATE" );
    
    if ( m_file->IsZombie() ){ 
        return; 
    }
        
    m_tree = new TTree( "Particles", "Particles");
    m_tree->Branch("Particle",&m_particlePtr); 
    m_tree->Branch("DeltaRay",&m_deltaPtr); 
    m_tree->Branch("Constants",&m_constants);
}

// Find the TrackedParticle object with a given trackID
const TrackedParticle* TrackNTupleSvc::getParticle( const size_t trackID ) const { 
    auto primaryIterator = m_primaryParticles.find( trackID );

    if ( primaryIterator != m_primaryParticles.end() ){
        return &primaryIterator->second; 
    }
    return nullptr;
}

// Add a particle to a collection organized by trackID
void TrackNTupleSvc::addParticle( const size_t trackID, 
                                  const TrackedParticle& particle ){ 
    m_primaryParticles[ trackID ] = particle; 
}

// Add a track segment to a collection organized by trackID
void TrackNTupleSvc::addTrackSegment( const size_t trackID,
                                 const TrackSegment& trackSegment ){
    m_trackSegments[ trackID ].emplace_back( trackSegment );
}

// Update the fields that are constant between all particles
void TrackNTupleSvc::addConstants(const pConstants constants){
    m_constants.magneticField = constants.magneticField;
    m_constants.mass = constants.mass;
    m_constants.charge = constants.charge;
}

// Order the track segments by increasing radial distance
void TrackNTupleSvc::sortTrackSegments(){ 
    for ( auto & [ id, segmentList ]: m_trackSegments ){
        std::sort( segmentList.begin(), segmentList.end(), 
                  []( const TrackSegment& a, 
                      const TrackSegment& b ){ 
        return ( a.getEntryPosition().Rho() < 
                 b.getEntryPosition().Rho() ); 
        });
    }
    return; 
}

// Add the collected particle and track segment data to the ROOT tree 
void TrackNTupleSvc::fillTree(){
    int count = 0;
    
    for ( const auto & [ id, particle ]: m_primaryParticles ){
        count++;
        m_particlePtr->Clear(); 
        m_deltaPtr->Clear();
        
        auto cur_ptr = m_particlePtr;
        if (particle.getIfDelta()) {
            cur_ptr = m_deltaPtr;
        }

        cur_ptr->setIfDelta( particle.getIfDelta() );
        cur_ptr->setID( particle.getID() );
        cur_ptr->setPosition( particle.getPosition() );
        cur_ptr->setMomentum( particle.getFourMomentum() );
        cur_ptr->clearTrackSegments();

        auto segmentIterator = m_trackSegments.find( id );
        if ( segmentIterator != m_trackSegments.end() ){
            cur_ptr->addTrackSegments( segmentIterator->second );
        }

        m_tree->Fill();
    }
    m_trackSegments.clear();
    m_primaryParticles.clear();
    
    G4cout << "\nNumber of particles filled in tree: " << count << G4endl;
    return;
}

} // namespace