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

namespace DriftChamberSim {

class KalmanFit {
public:
    struct Point {
        double x, y, z;
    };
    
    struct Intersection {
        bool valid = false;
        double t = 0.0;   
        Point p{0.0, 0.0, 0.0};
    };
    
    struct CellCrossing {
        bool crossed = false;
    
        Point entry{0.0, 0.0, 0.0};
        Point exit{0.0, 0.0, 0.0};
    
        double length = 0.0;
    };
    
    struct CellHit {
        xyzVector wirePos;
        xyzVector initMom;
        double length;
        int layer;
        int cell;
        Point entry;
        Point exit;
        double t1;
        double t2;
    };
    
    IndexMap GetLookupTable(std::string fileName);
    std::map<int, int> GetWiresPerLayer(); 
    
    HelixApproach GetHelix(CellHit cell);
    bool IsAngleInRange(double theta, double thetaMin, double thetaMax);
    bool IsPointInRegion(Point p, double rMin, double rMax, 
                         double thetaMin, double thetaMax);
    bool OnSegment(Point p, Point q, Point r);
    Intersection IsIntersect(Point a, Point b, Point c, Point d);
    std::vector<Intersection> IsIntersectArc(Point p1, Point p2, 
                         double R, double thetaMin, double thetaMax);
    CellCrossing IsInCell(LayerHit hit, double rMin, 
                         double rMax, double thetaMin, double thetaMax);
    
    std::vector<CellHit> GetDetectedWires(const std::vector<LayerHit>& sortedHits);
    void GetClusterInfo(std::vector<CellHit> detectedCells, IndexMap timeTable, IndexMap diffusionTable);
    void GetKalmanFit(std::vector<CellHit> detectedCells, int trackID, xyzVector initMomentum);
    void ProcessParticleTracks();

private:
    const double avgNumClusters = 109.68 /10.; // clusters per mm
    std::map<int, int> numWiresPerLayer;
    
    std::vector<KalmanHit> kalmanHits;
    
    xyzVector BField;
    double particleCharge;
    double particleMass;
};

} // namespace

#endif
