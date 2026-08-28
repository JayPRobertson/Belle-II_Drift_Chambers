#ifndef DCTrackedParticle_H
#define DCTrackedParticle_H 1

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
              const XYZVector& position, 
              const PxPyPzEVector& momentum,
              const bool isDelta); 
    
    TrackedParticle( const TrackedParticle& other ) = default; 

    TrackedParticle& operator=(const TrackedParticle& other) = default; 

    int getID() const { return m_id; }
    double getEnergy() const { return m_momentum.E(); }
    XYZVector getPosition() const { return m_position; }
    XYZVector getMomentum() const { return m_momentum.Vect(); }
    PxPyPzEVector getFourMomentum() const { return m_momentum; }
    bool getIfDelta() const { return m_isDelta; }

    void setID( const int id ){ m_id = id; }
    void setPosition( const XYZVector& position ){ m_position = position; }
    void setMomentum( const PxPyPzEVector& momentum ){m_momentum = momentum; }
    void setIfDelta( const bool isDelta ) { m_isDelta = isDelta; }
    
    void addTrackSegments( std::vector< TrackSegment >& segments ){
        m_trackSegments = segments; 
    }

    void clearTrackSegments(){
        m_trackSegments.clear();
    }

    const std::vector< TrackSegment >& getTrackSegments() const { 
        return m_trackSegments;
    }
    
    double getEnergyLoss() const; 
           
    struct ParticleConstants: public TObject {
        XYZVector magneticField;
        double mass;
        double charge;
        
        ParticleConstants() : mass(0), charge(0) {}
      
        XYZVector getBField() const { return magneticField; }
        double getMass() const { return mass; }
        double getCharge() const { return charge; }
        
        ClassDefNV(ParticleConstants, 1); 
    };

private: 
    int m_id = 0; 
    XYZVector     m_position{0,0,0}; 
    PxPyPzEVector m_momentum{0,0,0,0}; 
    
    bool m_isDelta = false;

    std::vector< TrackSegment > m_trackSegments; 

public:
    ClassDef( TrackedParticle, 1 );
};

} // namespace

#endif 