#ifndef DCReconstructedParticle_H
#define DCReconstructedParticle_H 1

#include "Math/Vector3D.h"
#include "TObject.h"

#include <vector>

using xyzVector = ROOT::Math::XYZVector;

namespace DriftChamberSim {

class ReconstructedParticle : public TObject { 
public:
    ReconstructedParticle() = default;

    ReconstructedParticle( const int id, 
              const xyzVector& position, 
              const xyzVector& momentum); 
    
    ReconstructedParticle( const ReconstructedParticle& other ) = default; 

    ReconstructedParticle& operator=(const ReconstructedParticle& other) = default; 
    
    struct LayerHit: public TObject {
        xyzVector entryPos;
        xyzVector entryMom;
        xyzVector exitPos;
        xyzVector exitMom;
        double entryTime;
        double exitTime;
        double edep;
        double initMomMag;
        int layerID;
        
        LayerHit() : entryTime(0), exitTime(0), edep(0), layerID(0){}
    
        xyzVector getEntryPosition() const { return entryPos; }
        xyzVector getExitPosition() const { return exitPos; }
        xyzVector getEntryMomentum() const { return entryMom; }
        xyzVector getExitMomentum() const { return exitMom; }
        double getEntryTime() const { return entryTime; }
        double getExitTime() const { return exitTime; }
        double getEnergyDeposition() const { return edep; }
        double getInitialMomentum() const { return initMomMag; }
        int getLayerID() const { return layerID; } 
        
        ClassDefNV(LayerHit, 1); 
    };

    struct KalmanHit: public TObject {
        xyzVector hitPos;
        xyzVector hitMom;
        double ndf;
        double chi2;
        double initMomMag;
            
        KalmanHit() : chi2(0), ndf(0) {}
        
        xyzVector getPosition() const { return hitPos; }
        xyzVector getMomentum() const { return hitMom; }
        double getChi2() const { return chi2; }
        double getDegFreedom() const { return ndf; }
        double getInitialMomentum() const { return initMomMag; }
        
        ClassDefNV(KalmanHit, 1);   
    };

    int getID() const { return m_id; }
    xyzVector getPosition() const { return m_position; }
    xyzVector getMomentum() const { return m_momentum; }
    
    const std::vector<LayerHit>& getActualHits() const { return m_actualHits; }
    const std::vector<KalmanHit>& getKalmanHits() const { return m_kalmanHits; }

    void setID( const int id ){ m_id = id; }
    void setPosition( const xyzVector& position ){ m_position = position; }
    void setMomentum( const xyzVector& momentum ){m_momentum = momentum; }
    
    void setActualHits( const std::vector<LayerHit> hit){ m_actualHits = hit; }
    void setKalmanHits( const std::vector<KalmanHit> hit){ m_kalmanHits = hit; }

private: 
    int m_id = 0; 
    xyzVector m_position{0,0,0};
    xyzVector m_momentum{0,0,0};

    std::vector<LayerHit> m_actualHits; 
    std::vector<KalmanHit> m_kalmanHits; 

public:
    ClassDef( ReconstructedParticle, 1 );
};

} // namespace

#endif 