#ifndef TRACKEDTrackedParticle_H
#define TRACKEDTrackedParticle_H 

#include "TrackSegment.hh"

#include "Math/Vector3D.h"
#include "Math/Vector4D.h"
#include "TObject.h"

#include <vector>

class TrackedParticle : public TObject { 
public:
    TrackedParticle() = default;

    TrackedParticle( const int id, 
              const ROOT::Math::XYZVector& position, 
              const ROOT::Math::PxPyPzEVector& momentum ); 
    
    TrackedParticle( const TrackedParticle& other ) = default; 

    TrackedParticle& operator=(const TrackedParticle& other) = default; 

    int getID() const { return m_id; }

    double getEnergy() const { return m_momentum.E(); }

    ROOT::Math::XYZVector getPosition() const {
        return m_position;
    }

    ROOT::Math::XYZVector getMomentum() const { 
        return m_momentum.Vect();
    }

    ROOT::Math::PxPyPzEVector getFourMomentum() const { 
        return m_momentum;
    }

    void setID( const int id ){ m_id = id; }

    void setPosition( const ROOT::Math::XYZVector& position ){ 
        m_position = position; 
    }

    void setMomentum( const ROOT::Math::PxPyPzEVector& momentum ){
        m_momentum = momentum;
    }

    void addTrackSegments( const std::vector< TrackSegment >& segments ){
        m_trackSegments = segments; 
    }

    void clearTrackSegments(){
        m_trackSegments.clear();
    }

    const std::vector< TrackSegment >& getTrackSegements() const { 
        return m_trackSegments;
    }

    double getEnergyLoss() const; 

private: 
    int m_id{0}; 
    ROOT::Math::XYZVector     m_position{0,0,0}; 
    ROOT::Math::PxPyPzEVector m_momentum{0,0,0,0}; 

    std::vector< TrackSegment > m_trackSegments; 

public:
    ClassDef( TrackedParticle, 1 );
};


#endif 