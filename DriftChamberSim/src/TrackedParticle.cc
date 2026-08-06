#include "TrackedParticle.hh"
#include <numeric>

namespace DriftChamberSim {

ClassImp(TrackedParticle)

TrackedParticle::TrackedParticle( const int id, 
                    const XYZVector& position, 
                    const PxPyPzEVector& momentum,
                    const bool isDeltaRay,
                    const double mass,
                    const XYZVector& magneticField) : 
    m_id( id ), 
    m_position( position ), 
    m_momentum( momentum ),
    m_isDelta( isDeltaRay ),
    m_mass( mass ),
    m_magneticField( magneticField ){};

double TrackedParticle::getEnergyLoss() const { 

    auto fn = []( double sum, const TrackSegment& segment ){ 
        return sum + segment.getEnergyLoss(); 
    };

    return std::accumulate( m_trackSegments.begin(), 
                            m_trackSegments.end(), 0., fn ); 
}

} // namespace