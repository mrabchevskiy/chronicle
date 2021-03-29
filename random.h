                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

 Random number generation
________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef RANDOM_H_INCLUDED
#define RANDOM_H_INCLUDED

#include <random>

#include "timer.h"

namespace CoreAGI {

  std::mt19937 randomNumber{ unsigned( Timer()( Timer::MICROSEC ) ) }; // :random number generator

}

#endif // RANDOM_H_INCLUDED
