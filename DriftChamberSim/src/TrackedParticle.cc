#include "TrackedParticle.hh"
#include <numeric>

namespace DriftChamberSim {

ClassImp(TrackedParticle)

TrackedParticle::TrackedParticle( const int id, 
                    const ROOT::Math::XYZVector& position, 
                    const ROOT::Math::PxPyPzEVector& momentum,
                    const bool isDeltaRay,
                    const double mass) : 
    m_id( id ), 
    m_position( position ), 
    m_momentum( momentum ),
    m_isDelta( isDeltaRay ),
    m_mass( mass ){};

double TrackedParticle::getEnergyLoss() const { 

    auto fn = []( double sum, const TrackSegment& segment ){ 
        return sum + segment.getEnergyLoss(); 
    };

    return std::accumulate( m_trackSegments.begin(), 
                            m_trackSegments.end(), 0., fn ); 
}

} // namespace