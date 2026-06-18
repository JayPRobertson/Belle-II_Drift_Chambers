#include "HelixApproach.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

#include <cmath>

namespace{
}

HelixApproach::HelixApproach(
    const G4ThreeVector& position,
    const G4ThreeVector& momentum,
    const G4ThreeVector& magneticField,
    G4double mass,
    G4double charge): fInitialPosition(position){
        
    fFieldAxis = magneticField.unit();

    G4double p = momentum.mag();
    G4double gamma = std::sqrt(p*p + mass*mass) / mass;
    G4double beta = p / std::sqrt(p*p + mass*mass);
    G4double speed = beta*c_light;
    G4double B = magneticField.mag();
    

    G4ThreeVector dir = RotateToFieldAxis(momentum.unit());

    fVparallel = speed*dir.z();
    fVperp = speed*dir.perp();
    
    if(B < 1e-20*tesla){
        fOmega = 0.;
        fRadius = DBL_MAX;
    }else{
        fOmega = charge*B*c_light/(gamma*mass);
        fRadius = gamma*mass*fVperp/(charge*B*c_light);
    }

    fAlpha = std::atan2(-dir.x(), dir.y());

    fHelixCentre = G4ThreeVector(
        -fRadius*std::cos(fAlpha),
        -fRadius*std::sin(fAlpha),
        0.0);
}

G4ThreeVector HelixApproach::RotateToFieldAxis(const G4ThreeVector& v) const {
    G4double ux = fFieldAxis.x();
    G4double uy = fFieldAxis.y();
    G4double uz = fFieldAxis.z();
    G4double rho = std::sqrt(ux*ux + uy*uy);

    if(rho < 1e-20) return v;

    return G4ThreeVector(
        ux*uz*v.x()/rho + uy*v.y()/rho - rho*v.z(),
        -uy*v.x()/rho + ux*v.y()/rho,
        ux*v.x() + uy*v.y() + uz*v.z());
}

G4ThreeVector HelixApproach::RotateFromFieldAxis(const G4ThreeVector& v) const {
    G4double ux = fFieldAxis.x();
    G4double uy = fFieldAxis.y();
    G4double uz = fFieldAxis.z();
    G4double rho = std::sqrt(ux*ux + uy*uy);

    if(rho < 1e-20) return v;

    return G4ThreeVector(
        ux*uz*v.x()/rho - uy*v.y()/rho + ux*v.z(),
        uy*uz*v.x()/rho + ux*v.y()/rho + uy*v.z(),
        -rho*v.x() + uz*v.z());
}

G4ThreeVector HelixApproach::Position(G4double t) const {
    G4ThreeVector shift(
        fRadius*std::cos(fOmega*t + fAlpha),
        fRadius*std::sin(fOmega*t + fAlpha),
        fVparallel*t);

    return fInitialPosition + RotateFromFieldAxis(fHelixCentre + shift);
}

G4ThreeVector HelixApproach::Velocity(G4double t) const {
    return RotateFromFieldAxis(
        G4ThreeVector(
            -fVperp*std::sin(fOmega*t + fAlpha),
             fVperp*std::cos(fOmega*t + fAlpha),
             fVparallel));
}

G4ThreeVector HelixApproach::Direction(G4double t) const {
    return Velocity(t).unit();
}


bool HelixApproach::InGas(
    const G4ThreeVector& p, G4double innerRadius, G4double outerRadius, G4double halfLength) const {
    
    G4double r = std::sqrt(p.x()*p.x() + p.y()*p.y());

    return (r >= innerRadius &&
            r <= outerRadius &&
            std::abs(p.z()) <= halfLength);
}

G4double HelixApproach::TimeAtCylinderRadius(G4double radius) const{
    if(radius > 2.0*std::abs(fRadius)) return -1.0;

    return 2.0*std::asin( radius/(2.0*std::abs(fRadius))) / std::abs(fOmega);
}


void HelixApproach::FindGasVolumeCrossings(
    G4double innerRadius,
    G4double outerRadius,
    G4double halfLength,
    G4ThreeVector& entryPoint,
    G4ThreeVector& exitPoint) const
{
    entryPoint = G4ThreeVector();
    exitPoint  = G4ThreeVector();

    G4double tEntry = TimeAtCylinderRadius(innerRadius);
    G4double tExit = TimeAtCylinderRadius(outerRadius);

    if(tEntry < 0 || tExit < 0) return;

    G4ThreeVector pEntry = Position(tEntry);
    G4ThreeVector pExit  = Position(tExit);

    entryPoint = pEntry;
    exitPoint = pExit;
}


