#include "HelixApproach.hh"

#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

namespace DriftChamberSim {

HelixApproach::HelixApproach(
    const G4ThreeVector& position,
    const G4ThreeVector& momentum,
    const G4ThreeVector& magneticField,
    G4double mass,
    G4double charge) : fInitialPosition(position) {
    
    fMomentum = momentum;
        
    G4double B = magneticField.mag();   // Magentic field strength
    G4double q = charge * eplus * -1;   // Particle charge
    G4double p = momentum.mag();        // Magnitude of momentum
    
    fEnergy = std::sqrt(p*p + mass*mass);   // Initial energy
    G4double beta = p / fEnergy;            // Particle speed as fraction of c
    fSpeed = beta * c_light;                // Particle speed

    fFieldAxis = magneticField.unit();      // Magnetic field axis
    
    G4ThreeVector dir = RotateToFieldAxis(momentum.unit());  // Momentum direction 

    fVparallel = fSpeed * dir.z();  // Particle velocity parallel to B field
    fVperp = fSpeed * dir.rho();    // Particle velocity perpendicular to B field
    
    // Cyclotron constants
    fOmega = (q * B * c_light * c_light) / fEnergy;          // Angular frequency
    fRadius = (p * dir.rho()) / (std::abs(q) * B*c_light);   // Rdius
    fAlpha = std::atan2(-dir.x(), dir.y());                  // Azimuthal angle

    fHelixCentre = G4ThreeVector(
        -fRadius * std::cos(fAlpha),
        -fRadius * std::sin(fAlpha),
        0.0);
}

// Rotates vector to magnetic field frame
G4ThreeVector HelixApproach::RotateToFieldAxis(const G4ThreeVector& v) const {
    G4double ux = fFieldAxis.x();
    G4double uy = fFieldAxis.y();
    G4double uz = fFieldAxis.z();
    G4double rho = std::sqrt(ux*ux + uy*uy);

    if(rho < 1e-30) return v;

    return G4ThreeVector(
        ux*uz*v.x()/rho + uy*v.y()/rho - rho*v.z(),
        -uy*v.x()/rho + ux*v.y()/rho,
        ux*v.x() + uy*v.y() + uz*v.z());
}

// Rotates vector to lab frame
G4ThreeVector HelixApproach::RotateFromFieldAxis(const G4ThreeVector& v) const {
    G4double ux = fFieldAxis.x();
    G4double uy = fFieldAxis.y();
    G4double uz = fFieldAxis.z();
    G4double rho = std::sqrt(ux*ux + uy*uy);

    if(rho < 1e-30) return v;

    return G4ThreeVector(
        ux*uz*v.x()/rho - uy*v.y()/rho + ux*v.z(),
        uy*uz*v.x()/rho + ux*v.y()/rho + uy*v.z(),
        -rho*v.x() + uz*v.z());
}

// Gets particle position at time t if travelling in helix
G4ThreeVector HelixApproach::Position(G4double t) const {
    G4ThreeVector shift(
        fRadius*std::cos(fOmega*t + fAlpha),
        fRadius*std::sin(fOmega*t + fAlpha),
        fVparallel*t);

    return fInitialPosition + RotateFromFieldAxis(fHelixCentre + shift);
}

// Gets particle velocity at time t if travelling in helix
G4ThreeVector HelixApproach::Velocity(G4double t) const {
    return RotateFromFieldAxis(
        G4ThreeVector(
            -fVperp*std::sin(fOmega*t + fAlpha),
             fVperp*std::cos(fOmega*t + fAlpha),
             fVparallel));
}

// Gets particle direction at time t if travelling in helix
G4ThreeVector HelixApproach::Direction(G4double t) const {
     return RotateFromFieldAxis(
        G4ThreeVector(
            -fVperp*std::sin(fOmega*t + fAlpha)/fSpeed,
             fVperp*std::cos(fOmega*t + fAlpha)/fSpeed,
             fVparallel/fSpeed));
}

// Gets the time a particle travelling in a helix would reach a position on the radius of a cylinder
G4double HelixApproach::TimeAtCylinderRadius(G4double radius) const {
    G4ThreeVector position = Position(0.0);
    G4ThreeVector velocity = Velocity(0.0);
    
    G4double vx = velocity.x();
    G4double vy = velocity.y();
    G4double x0 = position.x();
    G4double y0 = position.y();
    
    //  Solve quadratic equation for intercept of line with cylinder
    G4double a = vx*vx + vy*vy;
    G4double b = 2.0 * (x0*vx + y0*vy);
    G4double c = (x0*x0 + y0*y0) - (radius*radius); 
        
    G4double discriminant = b*b - 4.0*a*c;
    if (discriminant < 0) return -1.0;

    G4double tpos = ( -b + std::sqrt(discriminant) ) / (2.0*a);
    
    /* 
        Use Newton-Raphson method to solve for intercept where
            f(t) =  x(t)^2 + y(t)^2  - r^2 = 0
        with 
            t_{n+1} = t_n - f(t)/f'(t) 
    */
    
    G4double func = 0.0;
    G4int count = 0;
    G4int maxCount = 1000; 
    const G4double epsilon = 1e-5 * mm * mm; 

    do {
        if (count >= maxCount){
            G4cout << "ERROR: Count exceeded limit. Current tpos = " << tpos 
                   << ", energy = " << fEnergy 
                   << ", momentum = " << fMomentum << G4endl;
            return -1.0;
        }

        G4ThreeVector pos = Position(tpos); 
        G4ThreeVector vel = Velocity(tpos); 
        
        func = (pos.x()*pos.x() + pos.y()*pos.y()) - (radius*radius); 
        
        G4double deriv = 2.0 * (pos.x()*vel.x() + pos.y()*vel.y());

        if (std::abs(deriv) < 1e-12) return -1.0;

        tpos -= func / deriv; 
        count++;
    }
    while (std::abs(func) > epsilon);

    return tpos;
}

// Gets theoretical positions a particle enters and exits the gas volume
void HelixApproach::FindGasVolumeCrossings(
    G4double innerRadius,
    G4double outerRadius,
    G4double halfLength,
    G4ThreeVector& entryPoint,
    G4ThreeVector& exitPoint) const{
        
    entryPoint = G4ThreeVector();
    exitPoint  = G4ThreeVector();

    G4double tEntry = TimeAtCylinderRadius(innerRadius);
    G4double tExit = TimeAtCylinderRadius(outerRadius);

    // Exit if time is negative
    if(tEntry < 0 || tExit < 0) return;

    G4ThreeVector pEntry = Position(tEntry);
    G4ThreeVector pExit  = Position(tExit);

    entryPoint = pEntry;
    exitPoint = pExit;
}

} //namespace

