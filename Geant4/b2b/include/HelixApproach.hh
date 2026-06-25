#include "G4ThreeVector.hh"

class HelixApproach {
public:
    HelixApproach(
        const G4ThreeVector& position,
        const G4ThreeVector& momentum,
        const G4ThreeVector& magneticField,
        G4double mass,
        G4double charge);

    G4ThreeVector Position(G4double t) const;
    G4ThreeVector Velocity(G4double t) const;
    G4ThreeVector Direction(G4double t) const;
    G4double TimeAtCylinderRadius(G4double radius) const;

    void FindGasVolumeCrossings(
        G4double innerRadius,
        G4double outerRadius,
        G4double halfLength,
        G4ThreeVector& entryPoint,
        G4ThreeVector& exitPoint) const;

private:

    G4ThreeVector RotateToFieldAxis(const G4ThreeVector& v) const;
    G4ThreeVector RotateFromFieldAxis(const G4ThreeVector& v) const;

    G4ThreeVector fInitialPosition; // particle starting position
    G4ThreeVector fFieldAxis;       // axis of the magnetic field

    G4ThreeVector fHelixCentre;

    G4double fVparallel = 0.0;      // particle velocity parallel to B field
    G4double fVperp = 0.0;          // particle velocity perpendicular to B field
    G4double fOmega = 0.0;          // cyclotron angular frequency of particle
    G4double fRadius = 0.0;         // cyclotron radius
    G4double fAlpha = 0.0;          // cyclotron azimuthal angle
    G4double fSpeed = 0.0;          // particle speed
};