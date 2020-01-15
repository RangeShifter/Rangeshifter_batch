//---------------------------------------------------------------------------

#pragma hdrstop

#include "RSrandom.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

#if RSDEBUG
#include "Parameters.h"
extern paramSim *paramsSim;
#include <fstream>
//ofstream RSRANDOMLOG;
#endif

int RS_random_seed = 0;

// C'tor
// if parameter seed is negative, a random seed will be generated, else it is used as seed
RSrandom::RSrandom(int seed)
{
    // get seed
    int random_seed;
    if (seed < 0) {
        // random seed
        std::random_device device;
        random_seed = device();
        #if RSDEBUG
            DEBUGLOG << "RSrandom::RSrandom(): Generate random seed = ";
        #endif 
    }
    else{
        // fixed seed
        random_seed = seed;
        #if RSDEBUG
            DEBUGLOG << "RSrandom::RSrandom(): Use fixed seed = ";
        #endif 
    }
    #if RSDEBUG
        DEBUGLOG << random_seed << endl;
    #endif
    RS_random_seed = random_seed;
    
    // set up Mersenne Twister random number generator
    gen = new mt19937(random_seed);
    
    // Set up standard uniform distribution
    pRandom01 = new uniform_real_distribution<double> (0.0,1.0);
    // Set up standard normal distribution
    pNormal = new normal_distribution<double> (0.0,1.0);

}

RSrandom::~RSrandom(void) {
    delete gen;
    if (pRandom01 != 0) delete pRandom01;
    if (pNormal != 0) delete pNormal;
}

double RSrandom::Random(void) {
    // return random number between 0 and 1
    return pRandom01->operator()(*gen);
}

int RSrandom::IRandom(int min,int max) {
    // return random integer in the interval min <= x <= max
    uniform_int_distribution<int> unif(min,max);
    return unif(*gen);
}

int RSrandom::Bernoulli(double p) {
    return Random() < p;
}

double RSrandom::Normal(double mean,double sd) {
    return mean + sd * pNormal->operator()(*gen);
}

int RSrandom::Poisson(double mean) {
    poisson_distribution<int> poiss(mean);
    return poiss(*gen);
}


/* ADDITIONAL DISTRIBUTIONS

// Beta distribution - sample from two gamma distributions
double RSrandom::Beta(double p0,double p1) {
    double g0,g1,beta;
    if (p0 > 0.0 && p1 > 0.0) { // valid beta parameters
        gamma_distribution<double> gamma0(p0,1.0);
        gamma_distribution<double> gamma1(p1,1.0);
        g0 = gamma0(*gen);
        g1 = gamma1(*gen);
        beta = g0 / (g0 + g1);
    }
    else { // return invalid value
        beta = -666.0;
    }
    return beta;
}

// Gamma distribution
double RSrandom::Gamma(double p0,double p1) {  // using shape (=p0) and scale (=p1)
    double p2,gamma;
    if (p0 > 0.0 && p1 > 0.0) { // valid gamma parameters
        p2 = 1.0 / p1;
        gamma_distribution<double> gamma0(p0,p2);  // using shape/alpha (=p0) and rate/beta (=p2=1/p1)
        gamma = gamma0(*gen);
    }
    else { // return invalid value
        gamma = -666.0;
    }
return  gamma;
}

// Cauchy distribution
double RSrandom::Cauchy(double loc, double scale) { 
    double res;
    if (scale > 0.0) { // valid scale parameter
        cauchy_distribution<double> cauchy(loc,scale);
        res = cauchy(*gen);
    }
    else { // return invalid value
        res = -666.0;
    }
    return res;
}


*/

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
