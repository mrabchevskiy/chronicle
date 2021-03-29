                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

 Utility for base64-like encoding and decoding sequence of Identities

 2020.07.28
________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef CODEC_H_INCLUDED
#define CODEC_H_INCLUDED

#include <cstdio>
#include <cassert>
#include <cstdint>

#include <string>
#include <type_traits>

#include "def.h"  // :Identity definition

namespace CoreAGI {

  template< typename Type > class Encoded {

    static_assert( std::is_unsigned< Type >::value or std::is_floating_point< Type >::value );
    static_assert( sizeof( Type ) <= 64 );

    static constexpr unsigned CAPACITY{ 20 };
    static constexpr unsigned BASE    { 64 };
                                                                                                                              /*
    Map symbols from `SYMBOL` string to index in this string:
                                                                                                                              */
    static constexpr uint8_t M[ 256 ]{
      /*   0 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /*   8 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /*  16 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /*  24 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /*  32 */255 /* */, 255 /* */, 255 /* */, 255 /* */,  63 /*$*/, 255 /* */, 255 /* */, 255 /* */,
      /*  40 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /*  48 */  0 /*0*/,   1 /*1*/,   2 /*2*/,   3 /*3*/,   4 /*4*/,   5 /*5*/,   6 /*6*/,   7 /*7*/,
      /*  56 */  8 /*8*/,   9 /*9*/, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /*  64 */ 62 /*@*/,  36 /*A*/,  37 /*B*/,  38 /*C*/,  39 /*D*/,  40 /*E*/,  41 /*F*/,  42 /*G*/,
      /*  72 */ 43 /*H*/,  44 /*I*/,  45 /*J*/,  46 /*K*/,  47 /*L*/,  48 /*M*/,  49 /*N*/,  50 /*O*/,
      /*  80 */ 51 /*P*/,  52 /*Q*/,  53 /*R*/,  54 /*S*/,  55 /*T*/,  56 /*U*/,  57 /*V*/,  58 /*W*/,
      /*  88 */ 59 /*X*/,  60 /*Y*/,  61 /*Z*/, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /*  96 */255 /* */,  10 /*a*/,  11 /*b*/,  12 /*c*/,  13 /*d*/,  14 /*e*/,  15 /*f*/,  16 /*g*/,
      /* 104 */ 17 /*h*/,  18 /*i*/,  19 /*j*/,  20 /*k*/,  21 /*l*/,  22 /*m*/,  23 /*n*/,  24 /*o*/,
      /* 112 */ 25 /*p*/,  26 /*q*/,  27 /*r*/,  28 /*s*/,  29 /*t*/,  30 /*u*/,  31 /*v*/,  32 /*w*/,
      /* 120 */ 33 /*x*/,  34 /*y*/,  35 /*z*/, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 128 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 136 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 144 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 152 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 160 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 168 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 176 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 184 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 192 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 200 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 208 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 216 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 224 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 232 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 240 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
      /* 248 */255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */, 255 /* */,
    };

    char  text[ CAPACITY ];
    char* head;

    template< typename Unsigned > void encode( Unsigned u ){
      constexpr char EOS{ 0 }; // :End-Of-String
      *head = EOS;
      if( u == 0 ){
        *( --head ) = SYMBOL[ 0 ];
      } else {
        constexpr unsigned BASE_{ BASE - 1 };
        for(;;){
          const Unsigned r{ u & BASE_ };
          u >>= 6;
          if( ( r == 0 ) and ( u == 0 ) ) break;
          *( --head ) = SYMBOL[ r ];
        }
      }
    }

    template< typename Unsigned > void decode( Unsigned& u ) const {
      u = 0;
      char* s{ head };
      while( *s and not isspace( *s ) ){
        const char     c = *s;
        const Unsigned d{ M[ unsigned( c ) ] };
        assert( d < BASE );
        u *= BASE;
        u += d;
        s++;
      }
    }

  public:
                                                                                                                              /*
    Encoding:
                                                                                                                              */
    static constexpr const char SYMBOL[ BASE+1 ]{ "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@$" };

    explicit Encoded( Type u ): text{ 0 }, head{ text + 19 }{
      if constexpr( sizeof( u ) == 4 ){
        encode( reinterpret_cast< uint32_t& >( u ) );
        return;
      }
      if constexpr( sizeof( u ) == 8 ){
        encode( reinterpret_cast< uint64_t& >( u ) );
        return;
      }
      assert( false ); // :invalid argument type
    }

    explicit Encoded( const char* src, char* separator = nullptr ): text{ 0 }, head{ text }{
      constexpr char EOS       { 0    }; // :End-Of-String
      unsigned       i         { 0    };
      bool           skipSpaces{ true };
      const char*    symbol    { src  };
      for(;;){
        if( skipSpaces ){
          if( isspace( *symbol ) ){ symbol++; continue; }
          else skipSpaces = false;
        }
        if( isspace( *symbol ) or *symbol == EOS ){
          if( separator ) *separator = *symbol;
          return;
        }
        assert( M[ unsigned( *symbol ) ] < BASE ); // :invalid char in the encoded number?
        text[ i++ ] = *symbol;
        symbol++;
        assert( i < CAPACITY );                   // :too long encoded?
      }
    }

    explicit Encoded( FILE* src, char* separator = nullptr ): text{ 0 }, head{ text }{
                                                                                                                              /*
      Read from `src` file symbol by symbol upto space (inclusively) or enf-of-file:
                                                                                                                              */
      int      s         { 0    };
      unsigned i         { 0    };
      bool     skipSpaces{ true };
      for(;;){
        s = fgetc( src );
        const char symbol{ char( s ) };
        if( skipSpaces ){
          if( isspace( symbol ) ) continue;
          else skipSpaces = false;
        }
        if( isspace( symbol ) or s < 0 ){
          if( separator ) *separator = symbol;
          return;
        }
        assert( M[ unsigned( symbol ) ] < BASE ); // :invalid char in the encoded number?
        text[ i++ ] = symbol;
        assert( i < CAPACITY );                   // :too long encoded?
      }
    }

    explicit operator bool() const {
                                                                                                                              /*
      Returns `false` if textual representation is empty (for example
      after attempt to read from file after End-Of-File reached):
                                                                                                                              */
      constexpr char EOS{ 0 }; // :End-Of-String
      return *head != EOS;
    }

    const char* c_str() const { return head; }

    explicit operator std::string() const { return std::string( head ); }

    explicit operator std::string_view() const { return std::string_view( head ); }

    explicit operator Type() const {
      if constexpr( sizeof( Type ) == 4 ){
        uint32_t u;
        decode( u );
        return reinterpret_cast< Type& >( u );
      }
      if constexpr( sizeof( Type ) == 8 ){
        uint64_t u;
        decode( u );
        return reinterpret_cast< Type& >( u );
      }
      assert( false );
      return Type{}; // :dummy (never reached)
    }

  };// class Encoded
                                                                                                                              /*
  Utility function:
   - read encodeed entity ID`s from the file `src` up to `separator` of enf of file;
   - decodes ID;
   - return firts ID as result (or 0 in case of empty record);
   - call f(.) for each followed ID
                                                                                                                              */
  Identity readAndDecode( std::function< void( Identity ) >f, FILE* src, char EOL = '\n' ){
    assert( src );
                                                                                                                              /*
    Load first ID:
                                                                                                                              */
    char separator{ EOL };
    const Encoded< Identity > first{ src, &separator }; // :read first encoded ID
    if( not bool( first ) ) return NIHIL;
    Identity ID{ Identity( first ) };
                                                                                                                              /*
    Load rest of ID`s:
                                                                                                                              */
    if( separator != EOL ){ // Read encoded, append decoded to `seq`:
      for(;;){
        const Encoded< Identity > next{ src, &separator };
        if( not bool( next ) ) break;
        f( Identity( next ) );
        if( separator == EOL ) break;
      }
    }
    return ID;
  }

}//CoreAGI

#endif // CODEC_H_INCLUDED
