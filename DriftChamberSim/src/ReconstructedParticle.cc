#include "ReconstructedParticle.hh"

namespace DriftChamberSim {

ClassImp(ReconstructedParticle)

ReconstructedParticle::ReconstructedParticle( const int id, 
                    const xyzVector& position, 
                    const xyzVector& momentum
                    ) : 
    m_id( id ), 
    m_position( position ), 
    m_momentum( momentum ){};

} // namespace