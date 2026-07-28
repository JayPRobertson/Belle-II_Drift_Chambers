#include <vector>
#include <map>
#include "globals.hh"
#include "Math/Vector3D.h"

using xyzVector = ROOT::Math::XYZVector;

namespace B2 {

class KalmanFit {
public:
    struct Point {
        double x, y;
    };
    
    struct Intersection {
        bool valid = false;
        double t = 0.0;   
        Point p{0.0, 0.0};
    };
    
    struct CellCrossing {
        bool crossed = false;
    
        Point entry{0.0, 0.0};
        Point exit{0.0, 0.0};
    
        double length = 0.0;
    };
    
    struct CellHit {
        xyzVector wirePos;
        double length;
        int layer;
        int cell;
        Point entry;
        Point exit;
    };
    
    struct LayerHit {
        xyzVector entryPos;
        xyzVector initMom;
        xyzVector exitPos;
        xyzVector postMom;
        double entryTime;
        double exitTime;
        double edep;
        int layerID;
    };
    
    bool isAngleInRange(double theta, double thetaMin, double thetaMax);
    bool isPointInRegion(Point p, double rMin, double rMax, 
                         double thetaMin, double thetaMax);
    bool onSegment(Point p, Point q, Point r);
    Intersection isIntersect(Point a, Point b, Point c, Point d);
    std::vector<Intersection> isIntersectArc(Point p1, Point p2, 
                         double R, double thetaMin, double thetaMax);
    CellCrossing isInCell(Point p1, Point p2, double rMin, 
                         double rMax, double thetaMin, double thetaMax);
    
    std::map<int, int> GetWiresPerLayer(); 
    
    std::vector<xyzVector> GetDetectedWires(std::vector<LayerHit> sortedHits);
    void GetKalmanFit(std::vector<xyzVector> detectedWirePos, int trackID, xyzVector initMomentum);
    void ProcessParticleTracks();

private:
    const double avgNumClusters = 109.68 /10.; // clusters per mm
};

} // namespace B2 