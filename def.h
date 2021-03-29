                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

 Types definitions and some utilities

 2021.02.20
________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef DEF_H_INCLUDED
#define DEF_H_INCLUDED

#include <compare>
#include <limits>
#include <string>

namespace CoreAGI {

  using Identity = uint32_t; // :unsigned integer that keeps entity ID
  using Key      = uint64_t; // :unsigned integer that keep combination of two entity ID, object ID & attribute ID

  static_assert( sizeof( Key ) == 2*sizeof( Identity ) );
                                                                                                                              /*
  Hasher for Identity:
                                                                                                                              */
  struct IdentityHash {
    std::size_t operator()( const Identity& i ) const { return size_t( i ); }
  };

  Key combination( const Identity& object, const Identity& attribute ){
    constexpr size_t LEN{ 8*sizeof( Identity ) };
    Identity mask{ 0x1 };
    Key      key {   0 };
    for( size_t i = 0; i < LEN; i++ ){
      key  <<= 1;  if( mask & object    ) key += 1;
      key  <<= 1;  if( mask & attribute ) key += 1;
      mask <<= 1;
    }
    return key;
  }//combine

  constexpr const Identity    NIHIL { 0  };  // :identity of nonexistent quasi-entity
            const std::string NIL   { "" };  // :empty string


  struct Span{

    static_assert( std::numeric_limits< double >::has_infinity );

    double L;
    double R;

    Span(                    ): L{ -std::numeric_limits<double>::infinity() }, R{ std::numeric_limits<double>::infinity() }{}
    Span( double left        ): L{ left                                     }, R{ std::numeric_limits<double>::infinity() }{};
    Span( double a, double b ): L{ std::min( a, b )                         }, R{ std::max( a, b )                        }{};

    bool contains( double x ) const { return x >= L and x <= R; }

    auto operator <=> ( double x ) const {
      if( x < L ) return std::partial_ordering::less;
      if( x > R ) return std::partial_ordering::greater;
      else        return std::partial_ordering::equivalent;
    }

  };//struct Span


}//namespace CoreAGI

#endif // DEF_H_INCLUDED
