                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

 Timer for intervals in sec, millice, microsec, nonesec

 2020.05.04
________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include <cassert>

#include <chrono>
#include <thread>

namespace CoreAGI {

  void sleep( unsigned millisec ){
    std::this_thread::sleep_for( std::chrono::milliseconds( millisec ) );
  }

  class Timer {

    using Clock = std::chrono::steady_clock;
    using Time  = std::chrono::time_point< Clock >;

    Time to;
    Time tt;

  public:

    enum Unit: unsigned { NANOSEC = 0, MICROSEC = 1, MILLISEC = 2, SEC = 3 };

    static const char* lex( Unit unit ){
      switch( unit ){
        case NANOSEC : return "nanosec";
        case MICROSEC: return "microsec";
        case MILLISEC: return "millisec";
        case SEC     : return "sec";
        default: assert( false ); // :invalid argument
      }
      return "?";
    }

    static constexpr double UNIT[4] = { 1.0, 1.0e-3, 1.0e-6, 1.0e-9 };

    Timer(): to{ Clock::now() }, tt{ to }{ }

    void start(){ to = Clock::now(); tt = to; }
    void stop (){ tt = Clock::now();          }

    double elapsed( Unit unit = MILLISEC ){
      return UNIT[ unit ]*double( std::chrono::duration_cast< std::chrono::nanoseconds >( Clock::now() - to ).count() );
    }

    double operator()( Unit unit = MILLISEC ){
      return UNIT[ unit ]*double( std::chrono::duration_cast< std::chrono::nanoseconds >( tt - to ).count() );
    }

  };//Timer

}//namespace CoreAGI

#endif // TIMER_H_INCLUDED
