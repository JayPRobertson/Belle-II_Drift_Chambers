#ifndef RANDOMGENERATOR_H
#define RANDOMGENERATOR_H 1

#include <random>

namespace DriftChamberSim {

class RandomGenerator { 
    public:
        static RandomGenerator& instance(); 

        double getRandom(); 

        double fromUniform( const double range_min, const double range_max ); 
        
        double fromNormal( const double mu, const double sigma ); 

        double fromGamma ( const double alpha, const double beta ); 

        double fromExponential( const double lambda ); 

        int fromPoisson( const double mu ); 

        int fromRange( const int range_min, const int range_max ); 

        void setSeed( const unsigned seed );

    private:
        std::mt19937 m_random{ std::random_device()() }; 
};

} // namespace

#endif 