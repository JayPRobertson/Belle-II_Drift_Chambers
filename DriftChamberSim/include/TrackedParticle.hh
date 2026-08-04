#ifndef DCTrackedParticle_H
#define DCTrackedParticle_H 

#include "TrackSegment.hh"

#include "Math/Vector3D.h"
#include "Math/Vector4D.h"
#include "TObject.h"

#include <vector>

using XYZVector = ROOT::Math::XYZVector;
using PxPyPzEVector = ROOT::Math::PxPyPzEVector;

namespace DriftChamberSim {

class TrackedParticle : public TObject { 
public:
    TrackedParticle() = default;

    TrackedParticle( const int id, 
              const ROOT::Math::XYZVector& position, 
              const ROOT::Math::PxPyPzEVector& momentum,
              const bool isDelta,
              const double mass); 
    
    TrackedParticle( const TrackedParticle& other ) = default; 

    TrackedParticle& operator=(const TrackedParticle& other) = default; 

    int getID() const { return m_id; }
    double getEnergy() const { return m_momentum.E(); }
    XYZVector getPosition() const { return m_position; }
    XYZVector getMomentum() const { return m_momentum.Vect(); }
    PxPyPzEVector getFourMomentum() const { return m_momentum; }
    bool getIfDelta() const { return m_isDelta; }
    double getMass() const { return m_mass; }

    void setID( const int id ){ m_id = id; }
    void setPosition( const XYZVector& position ){ m_position = position; }
    void setMomentum( const PxPyPzEVector& momentum ){m_momentum = momentum; }
    void setIfDelta( const bool isDelta ) { m_isDelta = isDelta; }
    void setMass( const double mass) { m_mass =  mass; }

    void addTrackSegments( const std::vector< TrackSegment >& segments ){
        m_trackSegments = segments; 
    }

    void clearTrackSegments(){
        m_trackSegments.clear();
    }

    const std::vector< TrackSegment >& getTrackSegments() const { 
        return m_trackSegments;
    }
    
    double getEnergyLoss() const; 

private: 
    int m_id = 0; 
    ROOT::Math::XYZVector     m_position{0,0,0}; 
    ROOT::Math::PxPyPzEVector m_momentum{0,0,0,0}; 
    bool m_isDelta = false;
    double m_mass = 0.0;

    std::vector< TrackSegment > m_trackSegments; 

public:
    ClassDef( TrackedParticle, 1 );
};

} // namespace

#endif 