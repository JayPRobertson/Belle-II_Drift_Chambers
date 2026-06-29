#ifndef TRACKNTUPLESVC_H
#define TRACKNTUPLESVC_H

#include "TFile.h"
#include "TTree.h"

#include "TrackedParticle.hh"
#include "TrackSegment.hh"

#include <unordered_map> 
#include <string>
#include <vector> 

class TrackNTupleSvc { 
public:
    TrackNTupleSvc();
    ~TrackNTupleSvc(); 

    static TrackNTupleSvc& instance(); 
        
    void fileOpen( const std::string& fileName ); 
    void fileClose(); 

    void fillTree(); 

    void sortTrackSegments();

    void addParticle( const size_t trackID, 
                      const TrackedParticle& particle );

    void addTrackSegment( const size_t trackID, 
                          const TrackSegment& trackSegment ); 

    const TrackedParticle* getParticle( const size_t trackID ) const;
        
private:
    TFile* m_file{nullptr}; 
    TTree* m_tree{nullptr};

    TrackedParticle* m_particlePtr{nullptr}; 

    std::unordered_map< size_t, std::vector<TrackSegment> > m_trackSegments;
    std::unordered_map< size_t, TrackedParticle > m_primaryParticles;
};

#endif 