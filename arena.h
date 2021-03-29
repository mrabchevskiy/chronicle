                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

 ARENA MEMORY ALLOCATOR

 2021.03.20
________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef ARENA_H_INCLUDED
#define ARENA_H_INCLUDED
#include <cstring>

#include <memory>
#include <span>

namespace CoreAGI {

  class Arena {

    size_t   CAPACITY;  // :bytes
    uint8_t* SPACE;
    uint8_t* VACANT;
    size_t   AVAILABLE;

  public:

    Arena( unsigned capacity /* bytes */, uint8_t filler = 0 ):
      CAPACITY { capacity                     },
      SPACE    { (uint8_t*)malloc( capacity ) },
      VACANT   { SPACE                        },
      AVAILABLE{ capacity                     }
    {
      assert( SPACE );
      memset( SPACE, filler, CAPACITY );
    }

    Arena( const Arena& ) = delete;
    Arena& operator= ( const Arena& ) = delete;

    void reset( uint8_t filler = 0 ){
      VACANT    = SPACE;
      AVAILABLE = CAPACITY;
      memset( SPACE, filler, CAPACITY );
    }

    bool dump( const char* path = "arena.dump" ) const {
                                                                                                                                /*
      Write arena content into human readable text file
                                                                                                                                */
      FILE* out = fopen( path, "w" );
      if( out == nullptr ) return false;
      fprintf( out, " Arena capacity: %lu; occupied: %lu; vacant: %lu.\n", CAPACITY, CAPACITY - AVAILABLE, AVAILABLE );
      constexpr size_t COL_NUM{ 32 };
      const     size_t totalRows{ 1 + CAPACITY / COL_NUM  };
      for( size_t row = 0; row < totalRows; row++ ){
        fprintf( out, "\n %10lu ", row*COL_NUM );
        for( size_t col = 0; col < COL_NUM; col++ ){
          const size_t i{ row*COL_NUM + col };
          if( i < CAPACITY ) fprintf( out, " %02x", SPACE[i] ); else fprintf( out, "   " );
        }
      }
      fprintf( out, "\n" );
      fclose( out );
      return true;
    }

    size_t available() const { return AVAILABLE;            }
    size_t occupied () const { return CAPACITY - AVAILABLE; }

    template< typename T > T* settle( unsigned length = 1, unsigned alignTo = sizeof( T ) ){
      assert( length > 0 );
      unsigned total = length*sizeof( T );
      T* location{ (T*)std::align( alignTo, total, (void*&)VACANT, AVAILABLE ) };
      if( location ){
        VACANT    += total;
        AVAILABLE -= total;
      } else {
        printf( "\n\n [Arena.settle] FATAL ERROR: unsufficient available space:" );
        printf(   "\n   Available: %10lu Bytes", AVAILABLE );
        printf(   "\n   Requested: %10u Bytes",  total     );
        printf( "\n\n" );
        fflush( stdout );
        assert( false ); // :insufficient memory;
      }
      return location;
    }

    template< typename T > std::span< T > span( size_t length ){
      if( length == 0 ) return std::span< T >{};
                                                                                                                              /*
      Allocate T[ length ]:
                                                                                                                              */
      T* head{ settle< T >( length ) };
                                                                                                                              /*
      Initialize T[]:
                                                                                                                              */
      const T t{};
      for( unsigned i = 0; i < length; i++ ) head[i] = t;
      assert( head );
                                                                                                                              /*
      Return std::span that represents T[ length ]:
                                                                                                                              */
      return std::span< T >{ head, length };
    }

   ~Arena(){ free( SPACE ); }

  };

}//CoreAGI

#endif // ARENA_H_INCLUDED
