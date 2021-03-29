                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

 Event sequence - compacted temporal sequence of identities.
 Based on circular buffer equipped by specific methods.

 NIHIL (default value of Identity) of sequence element assumes missed/nonexistent one.


________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef CHRONICLE_H_INCLUDED
#define CHRONICLE_H_INCLUDED

#include <algorithm>
#include <functional>
#include <span>
#include <unordered_map>
#include <unordered_set>

#include "codec.h"
#include "def.h"
#include "flat.h"
#include "queue.h"


namespace CoreAGI {

  template< unsigned CAPACITY > class Chronicle {
  public:
                                                                                                                              /*
    Functions used for pattern processing:
                                                                                                                              */
    using Lex    = std::function< std::string( const Identity&                  ) >;
    using Act    = std::function< Identity   ( const Identity&, const Identity& ) >;
    using Sticky = std::function< bool       ( const Identity&, const Identity& ) >;

    static_assert( CAPACITY > 5, "Insufficient Chronicle capacity" );
                                                                                                                              /*
    Sequence element:
                                                                                                                              */
    struct Elem {

      Identity id;    // :ID of entity
      int32_t  prev;  // :location of the previous occurence of this ID or -1

      Elem(                                   ): id( NIHIL ), prev( -1          ){                            }
      Elem( const Identity& id                ): id( id    ), prev( -1          ){ assert( id != NIHIL     ); }
      Elem( const Identity& id, unsigned prev ): id( id    ), prev( int( prev ) ){ assert( prev < CAPACITY ); }

      bool operator == ( const Elem& e ) const { return id == e.id; }
      bool operator != ( const Elem& e ) const { return id != e.id; }

    };

  private:

    struct Ref{
                                                                                                                              /*
      Keep information about entity presented in the sequence: index of last occurence and
      index of previous occurence if entity presented twice or more times:
                                                                                                                              */
      unsigned last; // :location in `seq` of the last instance of referred entity [  0..CAPACITY-1 ]
      unsigned card; // :number of occurences of item in the `seq`

      Ref(                                  ): last{ 0        }, card{ 0    }{}
      Ref( unsigned location                ): last{ location }, card{ 1    }{}
      Ref( unsigned location, unsigned card ): last{ location }, card{ card }{}
      void push( unsigned location ){ last = location;    card++; }
      void pull(                   ){ assert( card > 0 ); card--; }
      unsigned num()   const { return card;      }
      bool     empty() const { return card == 0; }
    };

    using Seq = Queue    < Elem, CAPACITY >;
    using Loc = Flat::Map< Ref,  CAPACITY >;     // :location map: Identity -> Ref

    using Set = std::unordered_set< Identity >;  // :set of ID with modified location, so mapping must be redefined

    Lex lex;
                                                                                                                              /*
    Function that test suitability to form a new pattern of two consequtive subpatterns;
    returns NIHIL
                                                                                                                              */
    Sticky sticky;
                                                                                                                              /*
    Function that creates new pattern of two consequtive subpatterns; returns new Identity or NIHIL:
                                                                                                                              */
    Act make;
                                                                                                                              /*
    function that search/check for known pattern of two consequtive subpatterns;
    returns Identity of the known pattern of NIHIL:
                                                                                                                              */
    Act hunt;
                                                                                                                              /*
    Queue as underline container:
                                                                                                                              */
    Seq seq;
                                                                                                                              /*
    Maps identity presented in the sequence to head of the linked list of such ID in the sequence:
                                                                                                                              */
    Loc loc;
                                                                                                                              /*
    Number of NIHIL elements that replace removed ones in the sequence:
                                                                                                                              */
    unsigned holes; // :number of gaps in the sequence

    void mapLocation(){
                                                                                                                              /*
      Recreate `loc` and recalc `holes`:
                                                                                                                              */
      loc.clear();
      holes = 0;
      seq.process(
        [&]( Elem e, unsigned location )->bool{
          if( e.id == NIHIL ){ // Hole:
            holes++;
          } else { // Actual entity:
                                                                                                                              /*
            Items iterated from the head (oldest item) to tail (newest item), so
            each new occurence of the particular Identity just creates or updates `Locus`:
                                                                                                                              */
            Ref* R = loc[ e.id ];
            if( R ){ // Modify presented:
              e.prev  = R->last;
              R->last = location;
              R->card++;
            } else { // First occurence:
              assert( loc.incl( e.id, Ref{ location } ) > 0 );
            }
          }
          return true;
        }
      );
    }//mapLocation

    void push( const Identity& id ){

      if( id >= Flat::UINT24 ){
        printf( "\n\n [Chronicle.push] FATAL ERROR: Invalid id = %u >= UINT24 = %u\n\n", id, Flat::UINT24 );
        assert( false );
      }
                                                                                                                              /*
      Push new element into sequence and update `loc` including
      possible `loc` changes due expelling oldest sequence element
                                                                                                                              */
      auto updateExpelled = [&]( std::pair< Elem, Elem > duo ){
                                                                                                                              /*
        Note: `x` is an entity expelled from the `seq`, or NIHIL;
              `y` is a  oldest entity presented in `seq` after `x` expelled (can be NIHIL)
                                                                                                                              */
        auto [ x, y ] = duo;
        if( x.id != NIHIL ){
                                                                                                                              /*
          So `x` is an actual entity so list of `x` occurences should be updated
                                                                                                                              */
          Ref* Rx = loc[ x.id ];
          assert( Rx );
          assert( Rx->card > 0 );
          if( Rx->card == 1 ){
                                                                                                                              /*
            Expelled entity `x` was presented once so it should be just removed from the `loc`:
                                                                                                                              */
            loc.excl( x.id );
          } else {
                                                                                                                              /*
            Update list of `x` occurences;
            Note that expelled `x` may have same ID as newly inserted.
                                                                                                                              */
            int succ = Rx->last;
            int term = seq.lastLoc(); // : terminal index == index of the expelled item == index of the pushed item
            assert( term >= 0 );
            for(;;){
              int pred{ seq.ref( succ ).prev };
              if( pred == term ){
                seq.ref( succ ).prev = -1; // :end of linked list
                break;
              }
              succ = pred;
            }
            Rx->card--;
          }
        }
      };//updateExpelled

      if( loc.contains( id ) ){
                                                                                                                              /*
        Such ID already presented in the sequence:
                                                                                                                              */
        Ref* R = loc[ id ];
        assert( R );
        Elem e{ id, R->last };
        updateExpelled( seq.tamp( e ) );
        R->last = seq.lastLoc();
        R->card++;
      } else {
                                                                                                                              /*
        First occurence of ID:
                                                                                                                              */
        Elem e{ id };
        updateExpelled( seq.tamp( e ) );
        int lastLoc{ seq.lastLoc() }; // :negative means `not exists`
        assert( lastLoc >= 0 );
        Ref A{ unsigned( lastLoc ) };
        loc.incl( e.id, A );
        assert( loc[ e.id ]->card == 1 and loc[ e.id ]->last == unsigned( lastLoc ) ); // DEBUG
      }
    }

    Identity pop(){
      Elem e = seq.pop();
      if( e.id != NIHIL ){
        Ref* R = loc[ e.id ];
        assert( R );
        assert( R->card > 0 );
        if( R->card > 1 ){ // Not a last occurence:
          R->last = e.prev;
          R->card--;
        } else { // Last occurence:
          loc.excl( e.id );
        }
      }
      return e.id;
    }

  public:

    Chronicle( Lex lex, Sticky sticky, Act make, Act hunt ):
      lex   { lex    },
      sticky{ sticky },
      make  { make   },
      hunt  { hunt   },
      seq   {        },
      loc   {        },
      holes { 0      }
    {
      assert( lex    );
      assert( sticky );
      assert( make   );
      assert( hunt   );
    }

    Chronicle() = delete;
    Chronicle( const Chronicle& ) = delete;
    Chronicle& operator = ( const Chronicle& ) = delete;

    explicit operator bool() const { return not seq.empty(); }

    bool     empty   () const { return seq.empty();        }
    unsigned size    () const { return seq.size();         }  // :actual size including  excluded elements
    unsigned len     () const { return seq.size() - holes; }  // :logical length without excluded elements
    unsigned gap     () const { return holes;              }  // :number of              excluded elements
    unsigned distinct() const { return loc.size();         }  // :number of              distinct elements
    Elem     last    () const { return seq.last();         }  // :last element
    Identity lastId  () const { return seq.last().id;      }  // :last element`s ID
                                                                                                                              /*
    Clear sequence:
                                                                                                                              */
    void reset(){
      seq.clear();
      loc.clear();
      holes = 0;
      assert( empty()         );
      assert( distinct() == 0 );
    }
                                                                                                                              /*
    Reordering sequence to eliminate empty elements (time consiming action):
                                                                                                                              */
    unsigned compact(){ // returns number of eliminated empty elements:
      const unsigned n{ seq.compact() };
      holes = 0;
      mapLocation();
      return n;
    }

    unsigned num( const Identity& id ) const {
                                                                                                                              /*
      Location table contains only elements currently presented in the sequence;
      so if `id` not contained in `loc` are interpreted as num = 0:
                                                                                                                              */
      assert( id != CoreAGI::NIHIL );
      assert( id <  Flat::UINT24   );
      const Ref* R = loc[ id ];
      return R ? R->card : 0;
    }
                                                                                                                              /*
    Include identity into sequence:
                                                                                                                              */
    bool incl( const Identity& id ){
                                                                                                                              /*
      Check argument:
                                                                                                                              */
      if( id == NIHIL ){
        printf( "\n\n [Chronicle.incl] FATAL ERROR: id = %u\n\n", id ); fflush( stdout );
        return false;
      }
      if( id > Flat::UINT24 ){
        printf( "\n\n [Chronicle.incl] FATAL ERROR: too big id = %u > UINT24 = %u\n\n", id, Flat::UINT24 ); fflush( stdout );
        return false;
      }

      if( seq.empty() ){
        push( id );
        return true;
      }

      auto found = [&]( const Identity& pred, const Identity& succ )->std::tuple< int, int >{
                                                                                                                              /*
        Search for adjacent pair of presented occurences of `pred` and `succ`:
                                                                                                                              */
        Ref* Rp{ loc[ pred ] };                        assert( Rp );
        if( Rp->card < 2  ) return std::make_tuple( -1, -1 );
        int predIndex = Rp->last;
        int predShift { 0 };
        int predIndex_ = seq.ref( predIndex ).prev;
        if( predIndex_ > predIndex ){ assert( predShift == 0 ); predShift = CAPACITY; }
        predIndex = predIndex_;

        Ref* Rs{ loc[ succ ] };                        assert( Rs );
        if( Rs->card < 2 ){
          return std::make_tuple( -1, -1 );
        }
        int succIndex = Rs->last;
        int succShift{ 0 };
        int succIndex_ = seq.ref( succIndex ).prev;
        if( succIndex_ > succIndex ){ assert( succShift == 0 ); succShift = CAPACITY; }
        succIndex = succIndex_;

        do {
          assert( predIndex >= 0 );
          assert( succIndex >= 0 );

          if( seq.adjacent( predIndex, succIndex ) ) return std::make_tuple( predIndex, succIndex );

          if( ( succIndex - succShift ) > ( predIndex - predShift + 1 ) ){
            int predIndex_ = seq.ref( predIndex ).prev;
            if( predIndex_ > predIndex ){ assert( predShift == 0 ); predShift = CAPACITY; }
            predIndex = predIndex_;
          } else {
            int succIndex_ = seq.ref( succIndex ).prev;
            if( succIndex_ > succIndex ){
              if( succShift != 0 ){
                printf( "\n\n succShift=%u while expected to be 0\n", succShift ); fflush( stdout );
                assert( consistent() );
              }
              succShift = CAPACITY;
            }
            succIndex = succIndex_;
          }
        } while( predIndex >= 0 and succIndex >= 0 );
        return std::make_tuple( -1, -1 );
      };//found
                                                                                                                              /*
      Remember last pair of `seq` element that formed by pushing `e`;
      `pred` and `succ` here and further keep two last values of `seq`:
                                                                                                                              */
      Elem pred = last();   assert( pred.id != NIHIL );

      push( id );
      Elem succ = last();   assert( succ.id == id    );
                                                                                                                              /*
      Now `pred` an `succ` are two last elements of sequence;
      Check for known and/or new patterns:
                                                                                                                              */
      for(;;){

        auto replaceTwoLastByPattern = [&]( const Identity& pattern ){
                                                                                                                              /*
          Replace two last elements of the `seq` by the matched pattern;
          after such action sequence can contain just single element
          and `pred` can be `NIHIL`:
                                                                                                                              */
          assert( loc.size() >= 2 );
          pop();
          do pop(); while( last().id == NIHIL );
          pred = last();
          push( pattern );
          succ = last();
        };
                                                                                                                              /*
        Check if last two items matches some pattern:
                                                                                                                              */
        const Identity pattern = hunt( pred.id, succ.id );
        if( pattern != NIHIL ){
                                                                                                                              /*
          Note: pushing pattern into sequence by `push(.)` called from `replaceTwoLastByPattern(.)`
          updates loc[.].last and loc[.].card:
                                                                                                                              */
          assert( seq.size() >= 2 );
          replaceTwoLastByPattern( pattern );
          continue;
        }//if known pattern

        if( not sticky( pred.id, succ.id ) ) break;
        if( pred.id == succ.id ){
                                                                                                                              /*
          Make new pattern of two consecutive identical ID`s:
                                                                                                                              */
          Identity pattern = make( succ.id, succ.id ); assert( pattern != NIHIL );
          assert( loc[ pattern ] == nullptr ); // :no yet such pattern
                                                                                                                              /*
          Note: pushing pattern into sequence by `push(.)` called from `replaceTwoLastByPattern(.)`
          set correct loc[.].last and loc[.].card = 1:
                                                                                                                              */

          assert( loc.size() >= 2 );
          replaceTwoLastByPattern( pattern );
          loc[ pattern ]->card = 1;
          continue;
        }//if two identical
                                                                                                                              /*
        Search for two adjiacent elements of `seq` that matched [ last, e ] to make new pattern:
                                                                                                                              */
        auto [ Po, So ] = found( pred.id, succ.id );
        if( Po >= 0 and So >= 0 ){

          assert( Po < int( CAPACITY ) );
          assert( So < int( CAPACITY ) );
                                                                                                                              /*
          Make new pattern:
                                                                                                                              */
          const Identity pattern = make( pred.id, succ.id ); assert( pattern != NIHIL );
                                                                                                                              /*
          Replace first occurence by `NIHIL` and `pattern`.
          Traverse list of `pred` and remove node located at Po:
                                                                                                                              */
          Ref* Rp{ loc[ pred.id ] };                               assert( Rp );
          int P_ = Rp->last;
          int Pi = seq.ref( P_ ).prev;
          while( Pi != Po ){ P_ = Pi; Pi = seq.ref( Pi ).prev; }
          assert( Pi == Po and seq.ref( P_ ).prev == Po );
          seq.ref( P_ ).prev = seq.ref( Pi ).prev;
          seq.ref( Po ).prev = -1;
          seq.ref( Po ).id   = NIHIL;
          Rp->last = seq.ref( Rp->last ).prev;
          Rp->card -= 1;
                                                                                                                              /*
          Traverse list of `succ` and replace node located at So:
                                                                                                                              */
          Ref* Rs{ loc[ succ.id ] };                                assert( Rs );
          int S_ = Rs->last;
          int Si = seq.ref( S_ ).prev;
          while( Si != So ){ S_ = Si; Si = seq.ref( Si ).prev; }
          assert( Si == So and seq.ref( S_ ).prev == So );
          seq.ref( S_ ).prev = seq.ref( Si ).prev;
          seq.ref( So ).prev = -1;
          seq.ref( So ).id   = pattern;
          Rs->last = seq.ref( Rs->last ).prev;
          Rs->card -= 1;
          holes++;
                                                                                                                              /*
          Note: pushing pattern into sequence by `push(.)` called from `replaceTwoLastByPattern(.)`
          set loc[.].card = 1, so correction required:
                                                                                                                              */
          replaceTwoLastByPattern( pattern );
          Elem* last{ seq.lastRef() }; assert( last );
          assert( last->id   == pattern );
                                                                                                                              /*
          Link second pattern` occurence to first one and increase counter:
                                                                                                                              */
          last->prev = So;
          loc[ pattern ]->card = 2; // :correction required!
          continue;
        }//if found

        break;
      }//forever

      return true;
    }//incl
                                                                                                                              /*
    Check if sequence contains particular element at least once:
                                                                                                                              */
    bool contains( const Identity& id ) const { return loc.contains( id ); }
                                                                                                                              /*
    Call function `f` with identity as argument one by one starting from oldest one;
    process stopped if `f` returns `false`:
                                                                                                                              */
    bool process( std::function< bool( const Elem& e, unsigned i ) > f ) const { return seq.process( f ); }
                                                                                                                              /*
    Print sequence, so should nt be used in case of voluminous sequence:
                                                                                                                              */
    void expo() const {
      printf( "\n Chronicle sequence: len %u, size %u, gaps %u:", len(), size(), gap() );
      seq.process(
        [&]( const Elem& e, unsigned i )->bool{
          if     ( e.id == NIHIL ) printf( "\n %4u |" ,                     i                                    );
          else if( e.prev >= 0   ) printf( "\n %4u | %6i <- #%08u `%s`",    i, e.prev, e.id, lex( e.id ).c_str() );
          else                     printf( "\n %4u |           #%08u `%s`", i,         e.id, lex( e.id ).c_str() );
          return true;
        },
        true
      );
      printf( "\n Chronicle contains %u distinct entities:", distinct() );
      for( const auto& entry: loc )
          printf( "\n #%08u  last:%6i  card:%5i | `%s`", entry.key, entry.val.last, entry.val.card, lex( entry.key ).c_str() );
      printf( "\n" );
      fflush( stdout );
    };
                                                                                                                              /*
    Print statistics:
                                                                                                                              */
    void statistics() const {
      std::unordered_map< unsigned, unsigned > freq;
      unsigned maxlen{ 0 };
      for( const auto& entry: loc ){
        unsigned len = entry.val.card;
        if( len > maxlen ) maxlen = len;
        if( freq.contains( len ) ) freq[ len ] = freq[ len ] + 1; else freq[ len ] = 1;
      }
      std::vector< unsigned > H( maxlen + 1, 0 );
      for( const auto& it: freq ) H[ it.first ] = it.second;
      printf( "\n Histogram of frequency:" );
      for( unsigned len = 1; len < freq.size(); len++ ) if( H[len] > 0 ) printf( "\n %3u %4u", len, H[len] );
    }
                                                                                                                              /*
    Check data consistency (at debugging/testing stage):
                                                                                                                              */
    bool consistent() const {
                                                                                                                              /*
      Check `seq` links and presence of `seq` elements in the `loc`:
                                                                                                                              */
      unsigned err{ 0 };
      seq.process(
        [&]( const Elem& e, unsigned i )->bool{
          assert( e.id < Flat::UINT24 );
          if( not loc.contains( e.id ) ){
            printf( "\n Location table missed %u `%s`", i, lex( e.id ).c_str() );
            err++;
          }
          const int link = e.prev;
          if( link >= 0 ){
            if( link >= int( CAPACITY ) ){
              printf( "\n Invalid `seq` link %u `%s` -> %u >= CAPACITY = %u", i, lex( e.id ).c_str(), link, CAPACITY );
              err++;
            } else {
              const Elem& r = seq.ref( link );
              if( r.id != e.id ){
                printf( "\n Wrong link %u `%s` -> %u `%s`", i, lex( e.id ).c_str(), link, lex( r.id ).c_str() );
                err++;
              }
            }
          }
          return true;
        }
      );
                                                                                                                              /*
      Check `loc` for correct ID and counter:
                                                                                                                              */
      for( const auto& entry: loc ){
        Identity ID   = entry.key;
        Ref      R    = entry.val;
        unsigned len  = 0;
        int      link = R.last;
        if( link >= int( CAPACITY ) ){
            printf( "\n Invalid `loc` link `%s` -> %u >= CAPACITY = %u", lex( ID ).c_str(), link, CAPACITY );
            err++;
        } else {
          while( link >= 0 and link < int( CAPACITY ) ){
            len++;
            const Identity id = seq.ref( link ).id;
            if( id != ID ){
              printf( "\n Invalid ID in the linked list: `%s`..`%s`", lex( ID ).c_str(), lex( id ).c_str() );
              err++;
            }
            link = seq.ref( link ).prev; // :move to next list node
          }
          if( len != R.card ){
            printf( "\n Invalid `loc` card for `%s`: expected %i but actual equals %i", lex( ID ).c_str(), R.card, len );
            err++;
          }
        }//if link
      }//for entry
      if( err == 0 ) return true;
      printf( "\n Total %u inconsistencies detected", err );
      return false;
    }//consistent
                                                                                                                              /*
    Save data as a text file:
                                                                                                                              */
    bool save( const char* path ){
                                                                                                                              /*
      Saves only sequence of entity ID, so output not depends on capacity.
      Returns `false` if failed to open output file:
                                                                                                                              */
      FILE* out = fopen( path, "w" );
      if( not out ) return false;
      for( const Elem& e: seq.all() ){
        Encoded< Identity > key( e.id );
        fprintf( out, "%s\n", key.c_str() );
      }
      fclose( out );
      return true;
    }//save
                                                                                                                              /*
    Load data from the text file:
                                                                                                                              */
    int load( const char* path, std::function< bool( Identity ) > exist ){
                                                                                                                              /*
      Function `exist` checks if ID is known (presented in the semantic storage)

      Expands existing content by adding stored sequence one by one.

      Note: Use `reset()` if need before call `load()`.

      Returns  0 if run correctly
      Returns -1 if source file failed to open.
      Returns -2 if some ID == 0
      Returns unknown ID if such one presented; current Chronicle state not affected
      Returns -ID if incl(.) returns `false`; in such case current Chronicle state affected
                                                                                                                              */
      FILE* src = fopen( path, "r" );
      if( not src ) return false;
      std::vector< Identity > sequence;
      for(;;){
        Encoded< Identity > EncodedID( src );
        if( not bool( EncodedID ) ) break;
        Identity id{ Identity( EncodedID ) };
        if( id == CoreAGI::NIHIL ) return -2;
        if( not exist( id )      ) return id;
        sequence.push_back( id );
      }
      fclose( src );
      unsigned n{ 0 };
      for( const Identity& id: sequence ){
        if( not incl( id ) ) return -id;
        n++;
      }
      return 0;
    }//load

  };//class Chronicle

}//namespace CoreAGI

#endif
