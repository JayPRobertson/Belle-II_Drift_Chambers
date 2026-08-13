#ifdef __CLING__

#pragma link C++ nestedclasses;
#pragma link C++ nestedtypedefs;

// Namespaces
#pragma link C++ namespace DriftChamberSim;

// Simulation 
#pragma link C++ class DriftChamberSim::TrackedParticle+;
#pragma link C++ class DriftChamberSim::TrackSegment+;
#pragma link C++ class DriftChamberSim::TrackedParticle::ParticleConstants+;
#pragma link C++ class std::vector<DriftChamberSim::TrackedParticle>+;
#pragma link C++ class std::vector<DriftChamberSim::TrackSegment>+;
#pragma link C++ class std::vector<DriftChamberSim::TrackedParticle::ParticleConstants>+;

// Reconstruction
#pragma link C++ class DriftChamberSim::TrackedParticle::LayerHit+;
#pragma link C++ class DriftChamberSim::TrackedParticle::KalmanHit+;
#pragma link C++ class std::vector<DriftChamberSim::TrackedParticle::LayerHit>+;
#pragma link C++ class std::vector<DriftChamberSim::TrackedParticle::KalmanHit>+;


#endif
