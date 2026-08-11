#ifndef DCKalmanFit_h
#define DCKalmanFit_h 1

#include <vector>
#include <map>
#include "globals.hh"
#include "Math/Vector3D.h"
#include "TObject.h"

#include "TrackedParticle.hh"

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
    
    bool isAngleInRange(double theta, double thetaMin, double thetaMax);
    bool isPointInRegion(Point p, double rMin, double rMax, 
                         double thetaMin, double thetaMax);
    bool onSegment(Point p, Point q, Point r);
    Intersection isIntersect(Point a, Point b, Point c, Point d);
    std::vector<Intersection> isIntersectArc(Point p1, Point p2, 
                         double R, double thetaMin, double thetaMax);
    CellCrossing isInCell(LayerHit hit, double rMin, 
                         double rMax, double thetaMin, double thetaMax);
    
    std::pair<std::vector<xyzVector>, std::vector<CellHit>> GetDetectedWires(std::vector<LayerHit> sortedHits);
    void GetClusterInfo(std::vector<CellHit> detectedCells, IndexMap timeTable, IndexMap diffusionTable);
    void GetKalmanFit(std::vector<xyzVector> detectedWirePos, int trackID, xyzVector initMomentum);
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
