                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

 Queue with optional lock for push and/or pull.
 Based on circular buffer.

 2021.02.20
________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

#include <cassert>
#include <functional>

#include <atomic>
#include <limits>
#include <mutex>
#include <sstream>
#include <vector>
#include <utility>  // std::pair

namespace CoreAGI {

  enum class QueueNature { LOCK_FREE, LOCK_PUSH, LOCK_PULL, LOCK_FULL };

  template< typename Elem, unsigned CAPACITY, QueueNature NATURE = QueueNature::LOCK_FREE > class Queue {

    using Unsigned = unsigned;
    using Guard    = std::lock_guard< std::mutex >;

    const Elem              NIHIL;
    std::atomic< Unsigned > pushed;
    std::atomic< Unsigned > pulled;
    Elem                    seq[ CAPACITY ];
    std::mutex              mutex;

    static constexpr Unsigned MAX  { std::numeric_limits< Unsigned >::max() };
    static constexpr Unsigned LIMIT{ MAX - ( MAX % CAPACITY ) - CAPACITY    };
    static_assert( LIMIT % CAPACITY == 0   );
    static_assert( LIMIT + CAPACITY <  MAX );

    static unsigned location( unsigned pushed, unsigned pulled, int i ){  // [+] 2020.05.06
                                                                                                                              /*
      Caclculates index in the `seq` array of element by distance from the oldest presented
      element (which will be pulled at this moment) in case when `distance` >= 0 or from the
      last inserted element if distance < 0.
      So index(-1 ) refers last pushed element,
         index( 0 ) refers oldes presented element.
      If argument refers nonexisting element, returns unsigned(-1).
                                                                                                                              */
      const int SIZE{ int( pushed ) - int( pulled ) };
      unsigned result;
      if( i >= 0 ){ // Enumerate from first:
        while( i >= SIZE ) i -= CAPACITY;
        result = ( pulled + i ) % CAPACITY;
      } else { // Enumerate from last:
        while( abs( i ) > SIZE ) i -= CAPACITY;
        result = ( CAPACITY + pushed + i ) % CAPACITY;
      }
      assert( result < CAPACITY );
      return result;
    }

    Elem* element( unsigned i ){
      assert( i < CAPACITY );
      return seq[ i ];
    };

  public:

    Queue( const Elem& nihil = Elem{} ): NIHIL{ nihil }, pushed{ 0 }, pulled{ 0 }, seq{ NIHIL }, mutex{}{}

    Queue( const Queue& )              = delete;
    Queue& operator = ( const Queue& ) = delete;

    unsigned size()  const { return pushed.load() -  pulled.load(); }
    bool     empty() const { return pushed.load() == pulled.load(); }
    Elem     nihil() const { return NIHIL;                          }

    void clear(){
      if constexpr( NATURE != QueueNature::LOCK_FREE ) Guard lock( mutex );
      pushed.store( 0 );
      pulled.store( 0 );
    }

    bool adjacent( int i, int j ) const {
      for( int k = i;; ){
        if( ( ++k ) >= int( CAPACITY ) ) k = 0;
        if( k == j          ) return true;
        if( seq[k] != NIHIL ) return false;
      }
    }

    Elem* operator[] ( int i ){
                                                                                                                              /*
      Provides access to stored element by logical index (like Python`s list index):
      i == 0            refers oldest presented element,
      i == size() - 1   refers last inserted element,
      i == -1           refers penultimate element, and so on
                                                                                                                              */
      unsigned indx{ 0 };
      {
        if constexpr( NATURE != QueueNature::LOCK_FREE ) Guard lock( mutex );
        indx = location( pushed, pulled, i );
        assert( indx >= 0       );
        assert( indx < CAPACITY );
      }
      return &seq[ indx ];
    }

    const Elem* operator[] ( int i ) const { return (*this)[ i ]; }

    Elem& ref( unsigned i ){
      assert( i < CAPACITY );
      return seq[ i ];
    }

    const Elem& ref( unsigned i ) const {
      assert( i < CAPACITY );
      return seq[ i ];
    }

    Elem* lastRef(){
                                                                                                                              /*
      Returns last pushed (yungest) element or `NONE` of the sequence is empty:
                                                                                                                              */
      if constexpr( NATURE != QueueNature::LOCK_FREE ) Guard lock( mutex );
      if( pushed == pulled ) return nullptr;
      assert( pushed > pulled );
      return &seq[ ( pushed + CAPACITY - 1 ) % CAPACITY ];
    }

    Elem* nextToLastRef(){
                                                                                                                              /*
      Returns last pushed (yungest) element or `NONE` of the sequence is empty:
                                                                                                                              */
      if constexpr( NATURE != QueueNature::LOCK_FREE ) Guard lock( mutex );
      if( pushed == pulled ) return nullptr;
      assert( pushed > pulled );
      return &seq[ ( pushed + CAPACITY - 2 ) % CAPACITY ];
    }

    Elem last() const {
                                                                                                                              /*
      Returns last pushed (yungest) element or `NONE` of the sequence is empty:
                                                                                                                              */
      if constexpr( NATURE != QueueNature::LOCK_FREE ) Guard lock( mutex );
      if( pushed == pulled ) return NIHIL;
      assert( pushed > pulled );
      return seq[ ( pushed + CAPACITY - 1 ) % CAPACITY ];
    }

    Elem nextToLast() const {
                                                                                                                              /*
      Returns last pushed (yungest) element or `NONE` of the sequence is empty:
                                                                                                                              */
      if constexpr( NATURE != QueueNature::LOCK_FREE ) Guard lock( mutex );
      if( pushed == pulled ) return NIHIL;
      assert( pushed > pulled );
      return seq[ ( pushed + CAPACITY - 2 ) % CAPACITY ];
    }

    int lastLoc() const {
                                                                                                                              /*
      Returns index of the last pushed (youngest) element or -1 if the sequence is empty:
                                                                                                                              */
      if constexpr( NATURE != QueueNature::LOCK_FREE ) Guard lock( mutex );
      if( pushed == pulled ) return -1;
      assert( pushed > pulled );
      return ( pushed + CAPACITY - 1 ) % CAPACITY;
    }

    Elem first() const {
                                                                                                                              /*
      Returns oldest element (with logical position 0) or `NIHIL` if the sequence is empty
                                                                                                                              */
      if constexpr( NATURE != QueueNature::LOCK_FREE ) Guard lock( mutex );
      if( pushed == pulled ) return NIHIL;
      assert( pushed > pulled );
      return seq[ pulled % CAPACITY ];
    }

    Elem pull(){
                                                                                                                              /*
      In case of empty queue returns NIHIL:
                                                                                                                              */
      if constexpr( NATURE == QueueNature::LOCK_PUSH or NATURE == QueueNature::LOCK_FULL ) Guard lock( mutex );
      if( empty() ) return NIHIL;
      const unsigned i{ pulled.load() % CAPACITY };
      assert( i < CAPACITY );
      Elem e = seq[i];
      pulled++; // :atomic
      return e;
    }

    Elem pop(){
                                                                                                                              /*
      Removes and returns last pushed (youngest) element; returns NIHIL if sequence is empty:
                                                                                                                              */
      if constexpr( NATURE != QueueNature::LOCK_FREE ) Guard lock( mutex );
      if( pushed <= pulled ) return NIHIL;
      Elem e{ seq[ ( pushed + CAPACITY - 1 ) % CAPACITY ] };
      pushed--;
      return e;
    }

    bool push( const Elem& e ){
                                                                                                                              /*
      Returns `false` if pushing will cause overrun.
                                                                                                                              */
      assert( e != NIHIL );
      if( pushed.load() == LIMIT ){
                                                                                                                              /*
        Correct counters to avoid overflow:
                                                                                                                              */
        Guard lock( mutex );
        pushed.store( pushed.load() - LIMIT );
        pulled.store( pulled.load() - LIMIT );
      }
      if constexpr( NATURE == QueueNature::LOCK_PULL or NATURE == QueueNature::LOCK_FULL ) Guard lock( mutex );
      if( pushed.load() >= ( CAPACITY + pulled.load() ) ) return false; // :avoid overflow
      const unsigned i{ ( pushed.load() + CAPACITY ) % CAPACITY };
      assert( i < CAPACITY );
      seq[i] = e;
      pushed++; // :atomic
      return true;
    }

    std::pair< Elem, Elem > tamp( const Elem& e ){
                                                                                                                              /*
      Pushed element even if it requires to expell (pull) oldes one;
      returns pair composed by expelled element (or `NIHIL` if
      nothigh expelled) and logically next element (that still
      be in sequence):
                                                                                                                              */
      if constexpr( NATURE != QueueNature::LOCK_FREE ) Guard lock( mutex );
      assert( e != NIHIL );
      std::pair< Elem, Elem > result{ NIHIL, NIHIL }; // :result
      unsigned SIZE{ size() };
      assert( SIZE <= CAPACITY );
      if( SIZE == CAPACITY ){ // Expell oldest element:
        const unsigned i{ pulled.load() % CAPACITY };
        assert( i < CAPACITY );
        result.first = seq[i];
        pulled++; // :atomic
        result.second = seq[ pulled % CAPACITY ];
      }
      // push `e`:
      const unsigned i{ ( pushed.load() + CAPACITY ) % CAPACITY };
      assert( i < CAPACITY );
      seq[i] = e;
      pushed++; // :atomic
      return result;
    }

    unsigned compact(){
                                                                                                                              /*
      Remove all HIHIL elements ( NIHIL can't be inserted, but presented element can be
      replaced by NIHIL usign access by index); return number of removed empty elements:
                                                                                                                              */
      if constexpr( NATURE != QueueNature::LOCK_FREE ) Guard lock( mutex );
      const unsigned SIZE{ pushed - pulled };
      if( SIZE == 0 ) return 0;
      unsigned shift{ 0 };
      int indx{ 0 };
      for( unsigned i = 0; i < SIZE; i++ ){
        indx--;
        unsigned from = location( pushed, pulled, indx );  assert( from < CAPACITY );
        if( seq[ from ] == NIHIL ){
          shift++;
        } else {
          unsigned into = location( pushed, pulled, indx + shift );  assert( into < CAPACITY );
          seq[ into ] = seq[ from ];
        }
      }
      pulled += shift;
      return shift;
    }

    bool process( std::function< bool( const Elem& e, Unsigned i ) > f, bool includeHoles = false ) const {
                                                                                                                              /*
      Call f() for all queue elements sequentially;
      break if f() return `false`;
      returns `true` if all f() calls returned `true`:
                                                                                                                              */
      Unsigned p = pulled.load();
      while( p < pushed.load() ){
        const Unsigned i{ p % CAPACITY };
        assert( i < CAPACITY );
        const auto e{ seq[i] };
        if( e != NIHIL or includeHoles ) if( not f( e, i ) ) return false;
        p++;
      }
      return true;
    }

    std::vector< Elem > all() const {
                                                                                                                              /*
      Returns queue elements as vector; Lock-free, but no guarantee
      that result correctly represent partricular queue state:
                                                                                                                              */
      std::vector< Elem > A;
      process( [&]( Elem e, unsigned i )->bool{ A.push_back( e ); return true; } );
      return A;
    }

    operator std::string() const {
                                                                                                                              /*
      String representation:
                                                                                                                              */
      std::stringstream S;
      std::vector< Elem > A = all();
      if( A.empty() ) return std::string( "[]" );
      S << '[';
      for( const auto& e: A ) S << ' ' << e;
      S << ' ' << ']';
      return S.str();
    }

  };//Queue

}//namespace CoreAGI

#endif // QUEUE_H_INCLUDED
