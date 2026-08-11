#include "RandomGenerator.hh"

RandomGenerator& RandomGenerator::instance() { 
    static RandomGenerator instance;
    return instance; 
}

void RandomGenerator::setSeed( const unsigned seed ) { 
    m_random.seed( seed ); 
}

double RandomGenerator::getRandom() { 
    return m_random(); 
}

double RandomGenerator::fromUniform( const double range_min, 
                                     const double range_max ){ 
    std::uniform_real_distribution<double> distribution( range_min, range_max );
    return distribution( m_random ); 
}

double RandomGenerator::fromNormal( const double mu, 
                                    const double sigma ){ 
    std::normal_distribution<double> distribution( mu, sigma );
    return distribution( m_random ); 
}

double RandomGenerator::fromExponential( const double lambda ){ 
    std::exponential_distribution<double> distribution( lambda );
    return distribution( m_random ); 
}

double RandomGenerator::fromGamma( const double alpha, 
                                   const double beta ){ 
    std::gamma_distribution<double> distribution( alpha, beta );
    return distribution( m_random ); 
}

int RandomGenerator::fromPoisson( const double mu ){ 
    std::poisson_distribution<int> distribution( mu );
    return distribution( m_random );
}

int RandomGenerator::fromRange( const int range_min, 
                                const int range_max ){
    std::uniform_int_distribution<> distribution( range_min, range_max );
    return distribution( m_random );
}