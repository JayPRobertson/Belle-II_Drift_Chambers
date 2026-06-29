#include "TrackNTupleSvc.hh"
#include <algorithm> 
#include "G4ios.hh"

ClassImp(TrackNTupleSvc)

TrackNTupleSvc::TrackNTupleSvc() : m_particlePtr( new TrackedParticle ) {}

TrackNTupleSvc::~TrackNTupleSvc(){
    delete m_particlePtr;
}

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

void TrackNTupleSvc::fileOpen( const std::string& fileName ){
    m_file = new TFile( fileName.c_str(), "RECREATE" );
    
    if ( m_file->IsZombie() ){ 
        return; 
    }
        
    m_tree = new TTree( "Particles", "Particles");
    m_tree->Branch("Particle",&m_particlePtr); 
}


const TrackedParticle* TrackNTupleSvc::getParticle( const size_t trackID ) const { 
    auto primaryIterator = m_primaryParticles.find( trackID );

    if ( primaryIterator != m_primaryParticles.end() ){
        return &primaryIterator->second; 
    }
    return nullptr;
}

void TrackNTupleSvc::addParticle( const size_t trackID, 
                                  const TrackedParticle& particle ){ 
    m_primaryParticles[ trackID ] = particle; 
}

void TrackNTupleSvc::addTrackSegment( const size_t trackID,
                                 const TrackSegment& trackSegment ){
    m_trackSegments[ trackID ].emplace_back( trackSegment );
}

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

void TrackNTupleSvc::fillTree(){ 
    for ( const auto & [ id, particle ]: m_primaryParticles ){

        m_particlePtr->setID( particle.getID() );
        m_particlePtr->setPosition( particle.getPosition() );
        m_particlePtr->setMomentum( particle.getFourMomentum() );

        m_particlePtr->clearTrackSegments(); 

        auto segmentIterator = m_trackSegments.find( id );    
        if ( segmentIterator != m_trackSegments.end() ){
            m_particlePtr->addTrackSegments( segmentIterator->second ); 
        }

        m_tree->Fill();
    }

    m_trackSegments.clear();
    m_primaryParticles.clear();
    return; 
}
