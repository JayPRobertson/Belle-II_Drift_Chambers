#ifndef DCKalmanFit_h
#define DCKalmanFit_h 1

#include <vector>
#include <map>
#include "globals.hh"
#include "Math/Vector3D.h"
#include "TObject.h"

#include "TrackedParticle.hh"
#include "HelixApproach.hh"

using xyzVector = ROOT::Math::XYZVector;
using LayerHit = DriftChamberSim::TrackedParticle::LayerHit;
using KalmanHit = DriftChamberSim::TrackedParticle::KalmanHit;
using IndexMap = std::vector<std::vector<std::string>>;
using HitData = std::vector<std::pair<xyzVector, std::pair<xyzVector, xyzVector>>>;

namespace DriftChamberSim {

class KalmanFit {
public:
    struct WireInfo {
        double wireLength;
        xyzVector end1;
        xyzVector end2;
    };
    
    struct CellHit {
        xyzVector wirePos;
        xyzVector initMom;
        int layer;
        int cell;
        double t1;
        double t2;
        xyzVector entry;
        xyzVector exit;
        WireInfo wireInfo;
    };
    
    // Helper functions
    IndexMap GetLookupTable(std::string fileName);
    bool AreVectorsEqual(xyzVector pA, xyzVector pB);
    double GetDistance(xyzVector pA, xyzVector pB); 
    HelixApproach GetHelix(CellHit cell);
    
    // Analysis support functions
    std::map<int, std::tuple<int, double, std::string>> GetWiresPerLayer();
    xyzVector GetPointOfClosestApproach(xyzVector p1, xyzVector trackPoint, xyzVector wireVec);
    HitData GetClosestWire(LayerHit hit);
    
    // Analysis functions
    std::vector<CellHit> GetDetectedWires(const std::vector<LayerHit>& sortedHits);
    //void GetClusterInfo(std::vector<CellHit> detectedCells, IndexMap timeTable, IndexMap diffusionTable);
    void GetKalmanFit(std::vector<CellHit> detectedCells, int trackID, xyzVector initMomentum);
    void ProcessParticleTracks();

private:
    const double avgNumClusters = 109.68 /10.; // clusters per mm
    std::map<int, std::tuple<int, double, std::string>> numWiresPerLayer;
    
    std::vector<KalmanHit> kalmanHits;
    
    xyzVector BField;
    double particleCharge;
    double particleMass;
};

} // namespace

#endif
