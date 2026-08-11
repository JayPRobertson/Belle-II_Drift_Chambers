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
           
    struct LayerHit: public TObject {
        XYZVector entryPos;
        XYZVector initMom;
        XYZVector exitPos;
        XYZVector postMom;
        double entryTime;
        double exitTime;
        double edep;
        int layerID;
        
        LayerHit() : entryTime(0), exitTime(0), edep(0), layerID(0) {}
    
        XYZVector getEntryPosition() const { return entryPos; }
        XYZVector getExitPosition() const { return exitPos; }
        XYZVector getEntryMomentum() const { return initMom; }
        XYZVector getExitMomentum() const { return postMom; }
        double getEntryTime() const { return entryTime; }
        double getExitTime() const { return exitTime; }
        double getEnergyDeposition() const { return edep; }
        int getLayerID() const { return layerID; } 
        
        ClassDefNV(LayerHit, 1); 
    };

    struct KalmanHit: public TObject {
        XYZVector hitPos;
        XYZVector hitMom;
        double chi2;
        int trackID;
        double ndf;
            
        KalmanHit() : chi2(0), trackID(0), ndf(0) {}
    
        XYZVector getPosition() const { return hitPos; }
        XYZVector getMomentum() const { return hitMom; }
        double getChi2() const { return chi2; }
        int getTrackID() const { return trackID; }
        double getDegFreedom() const { return ndf; }
        
        ClassDefNV(KalmanHit, 1);   
    };
    
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