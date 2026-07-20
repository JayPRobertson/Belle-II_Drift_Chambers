#include "TrackSegment.hh"

ClassImp(TrackSegment)

TrackSegment::TrackSegment( const XYZVector& entryPos, 
                            const XYZVector& entryMom,
                            const XYZVector& exitPos,  
                            const XYZVector& exitMom,
                            const double entryTime,
                            const double energyLoss ) : 
    m_entryPos( entryPos ), 
    m_entryMom( entryMom ), 
    m_exitPos ( exitPos  ), 
    m_exitMom ( exitMom  ), 
    m_entryTime ( entryTime  ),
    m_energyLoss( energyLoss ){}

