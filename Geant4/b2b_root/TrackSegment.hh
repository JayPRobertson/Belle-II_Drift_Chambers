#ifndef TRACKSEGMENT_H
#define TRACKSEGMENT_H

#include "Math/Vector3D.h"
#include "TObject.h"

using XYZVector = ROOT::Math::XYZVector;

class TrackSegment : public TObject { 
public:
    TrackSegment() = default; 

    TrackSegment( const TrackSegment& other ) = default;  

    TrackSegment& operator=(const TrackSegment&) = default;

    TrackSegment( const XYZVector& entryPos, 
                  const XYZVector& entryMom,
                  const XYZVector& exitPos,  
                  const XYZVector& exitMom,
                  const double entryTime, 
                  const double energyLoss ); 

    void setEntryPosition( const XYZVector& position ){ m_entryPos = position; }
    void setEntryMomentum( const XYZVector& momentum ){ m_entryMom = momentum; }
    void setEntryTime( const double time ){ m_entryTime = time; }
    
    XYZVector getEntryPosition() const { return m_entryPos; }
    XYZVector getEntryMomentum() const { return m_entryMom; }
    double getEntryTime() const { return m_entryTime; }
    
    void setExitPosition( const XYZVector& position ){ m_exitPos = position; }
    void setExitMomentum( const XYZVector& momentum ){ m_exitMom = momentum; }
    void setEnergyLoss( const double energy ) { m_energyLoss = energy; }

    XYZVector getExitPosition() const { return m_exitPos; }
    XYZVector getExitMomentum() const { return m_exitMom; }
    double getEnergyLoss() const { return m_energyLoss; }


private:
    XYZVector m_entryPos{ 0., 0., 0. }; 
    XYZVector m_entryMom{ 0., 0., 0. };

    XYZVector m_exitPos{ 0., 0., 0. }; 
    XYZVector m_exitMom{ 0., 0., 0. };
    
    double m_entryTime{0};
    double m_energyLoss{0};  

public:
    ClassDef( TrackSegment, 1 ); 
};

#endif 