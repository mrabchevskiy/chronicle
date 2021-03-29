                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________
  Prototype:

    https://www.lewuathe.com/robin-hood-hashing-experiment.html
    https://github.com/Lewuathe/robinhood/blob/master/robinhood.go
  _________________________________________________________

  `Flat` hash table (contigous memory, no memory allocations when new element inserted)


  2020.12.03
________________________________________________________________________________________________________________________________
                                                                                                                              */
#ifndef FLAT_H_INCLUDED
#define FLAT_H_INCLUDED

#include <cassert>
#include <cstdint>
#include <cstring>

#include <utility>      // std::move
#include <vector>
#include <span>
#include <string>
#include <sstream>
#include <iomanip>
                                                                                                                              /*
NB Suppress warning:
                                                                                                                              */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"

namespace CoreAGI::Flat {

  constexpr uint32_t NIHIL { 0       };
  constexpr uint32_t UINT24{ 1 << 24 };

  using Elem = uint32_t; // :element of the Set
  using Key  = uint32_t; // :key     of the Map

  template< unsigned CAPACITY, unsigned LOAD_FACTOR_PERCENT = 80 > class Set {

    static constexpr unsigned SPACE = CAPACITY*100/LOAD_FACTOR_PERCENT;

    static_assert( std::is_trivially_copyable< Elem >::value );

  public:

    struct Entry {
      Elem    key : 24;
      uint8_t dib :  7;
      bool    del :  1;
    };

    enum Note: int8_t {
                                                                                                                              /*
      Explanation about incl/excl operation;
      Zero value means error, any other value means success
                                                                                                                              */
      EXHAUSTED = 0, // :not inserted because capacity exceeded
      INCLUDED  = 1, // :OK, included
      EXCLUDED  = 2, // :OK, excluded
      RECOVERED = 3, // :inserted element that was deleted earlier
      CONTAINED = 4, // :not inserted because already presented
      NOT_FOUND = 5, // :not excluded because not presented
      EMPTY_SET = 6  // :not excluded/included because empty set
    };

    static const char* lex( Note note ){
      switch( note ){
        case EXHAUSTED : return "EXHAUSTED";
        case INCLUDED  : return "INCLUDED ";
        case EXCLUDED  : return "EXCLUDED ";
        case RECOVERED : return "RECOVERED";
        case CONTAINED : return "CONTAINED";
        case NOT_FOUND : return "NOT_FOUND";
        case EMPTY_SET : return "EMPTY_SET";
        default        : break;
      }
      return nullptr;
    }

  private:

	  uint32_t cardinal;
	  Entry    data[ SPACE ];

  public:

	  std::string content() const {
	    std::stringstream out;
	    out << std::setw( 4 ) << cardinal << " {";
	    for( const auto& e: data ){
	      if( e.key == 0 ) out << "  empty ";
	      else if( e.del ) { out << " (" << std::setw( 3 ) << e.key << ')' << '`' << unsigned( e.dib ); }
	      else             { out << "  " << std::setw( 3 ) << e.key << ' ' << '`' << unsigned( e.dib ); }
	    }
	    out << " }";
	    return out.str();
	  }//content

  private:

    Note rehash( Entry e, unsigned depth ){
                                                                                                                              /*
      Collect all currently presented elements into the temporary array E:
                                                                                                                              */
      Elem E[ SPACE ];
      unsigned n{ 0 };
      for( unsigned i = 0; i < SPACE; i++ ) if( data[i].key != 0 and not data[i].del ) E[n++] = data[i].key;
      clear();
      for( unsigned i = 0; i < n; i++ ) assert( incl( E[i], depth + 1 ) );
                                                                                                                              /*
      Finally add `elem` if any:
                                                                                                                              */
      if( e.key and not e.del ) assert( incl_( e.key, depth + 1 ) == INCLUDED );
      return INCLUDED;

    }

	  Note incl_( Elem elem, unsigned depth = 0 ){
	    assert( elem  < UINT24 );
	    assert( depth < 2      );
	    if( cardinal >= CAPACITY ) return EXHAUSTED;
      unsigned i{ elem % SPACE                  }; // :desired position
      Entry    e{ key: elem, dib: 0, del: false }; // :DIB (distance to initial bucket) is zero initially
      unsigned step{ 0 };
      for( unsigned c = i; ; c = ( c + 1 ) % SPACE ){
		    if( data[c].key == 0 ){ // Vacant cell, insert here
			    data[c] = e;
			    cardinal++;
			    return INCLUDED;
		    }
		    if( data[c].key == e.key ){ // Presented or deleted
			    if( data[c].del ){ // Sometime was presented but deleted - just recovery:
			      data[c].del = false;
			      cardinal++;
			      return RECOVERED;
			    } else { // Presented:
			      return CONTAINED;
			    }
			    break;
		    }
		    step++;
			  if( data[c].dib < e.dib ){ // To be swapped because it is rich.
			                                                                                                                        /*
			    Swap `e` and `data[c]`, i.e. insert here but move current element to some another place
			                                                                                                                        */
          std::swap( e, data[c] );
        }//if
        constexpr unsigned DIB_LIMIT{ 7 }; // :rehashing criterion
        if( e.dib >= DIB_LIMIT or step > SPACE/2 ){ return rehash( e, depth ); }
		    else e.dib++; //:the entry to be inserted goes away from ideal position gradually
      }// for c
		  return INCLUDED;
	  }//incl

  public:

    void clear(){
      cardinal = 0;
	    memset( data, 0, sizeof( Entry )*SPACE );
    }

    Set(): cardinal{ 0 }, data{}{ memset( data, 0, sizeof( Entry )*SPACE );	}

    bool contains( const Elem elem ) const {
	    assert( elem != NIHIL );
	    assert( elem < UINT24 );
	    if( cardinal == 0  ) return false;
      unsigned i{ elem % SPACE };
	    while( data[i].key != elem ){
		    i = ( i + 1 ) % SPACE;
		    if( i == elem % SPACE ) return false;	// :not Found, vacant call reached
      }
                                                                                                                              /*
      Key found, but it may be deleted, so result depends on date[.].del:
                                                                                                                              */
	    return not data[i].del;
    }

    bool contains( const std::span< Elem > elem ) const {
      if( elem.size() > cardinal ) return false;
      for( const auto& e: elem ) if( not contains( e ) ) return false;
      return true;
    }

    Note excl( const Elem elem ){
	    assert( elem != NIHIL );
	    assert( elem < UINT24 );
	    if( cardinal == 0 ) return EMPTY_SET;
      unsigned i{ elem % SPACE };
	    while( data[i].key != elem ){
		    i = ( i + 1 ) % SPACE;
		    if( i == elem % SPACE ){
		      return NOT_FOUND;	// :not Found
		    }
      }
                                                                                                                              /*
      Key found, mark it as deleted:
                                                                                                                              */
      if( not data[i].del ){
        data[i].del = true;
        cardinal--;
        return EXCLUDED;
      }
      return NOT_FOUND; // :the item has already been deleted earlier
    }

    Note incl( Elem elem, unsigned depth = 0 ){ return incl_( elem, depth ); }

	  Set( std::initializer_list< Elem > E ): cardinal{ 0 }, data{}{
      memset( data, 0, sizeof( Entry )*SPACE );
	    for( const auto& e: E ) incl( e );
	  }

    Set& operator = ( std::initializer_list< Elem > E ){
      clear();
      for( auto& e:E ) incl( e );
      return *this;
    }

    Set& operator += ( std::initializer_list< Elem > E ){
      for( auto& e:E ) incl( e );
      return *this;
    }

    Set& operator += ( const Elem e ){ incl( e ); return *this; }
    Set& operator -= ( const Elem e ){ excl( e ); return *this; }

	  explicit operator bool() const{ return cardinal > 0; }

    bool operator[] ( const Elem e ) const { return contains( e ); }

	  unsigned size () const { return cardinal;      }
	  bool     empty() const { return cardinal == 0; }

    bool operator == ( const Set& M ) const {
      if( size() != M.size() ) return false;
      for( const auto& e: M ) if( not contains( e ) ) return false;
      return true;
    }

    bool contains( const Set& M ) const {
                                                                                                                              /*
      Returns `true` if M is a subset of this or equal to this; if M or this is empty, return `false`:
                                                                                                                              */
      if( empty() or M.empty() ) return false;
      if( size() < M.size()    ) return false;
      for( const auto& e: M ) if( not contains( e ) ) return false;
      return true;
    };

    bool operator >= ( const Set& M ) const { return contains( M ); }

    bool containedIn( const Set& M ) const {
                                                                                                                              /*
      Returns `true` if this is a subset of M ot equal M; if M or this is empty, return `false`:
                                                                                                                              */
      if( empty() or M.empty() ) return false;
      if( size() > M.size()    ) return false;
      for( const auto& e: (*this) ) if( not M.contains( e ) ) return false;
      return true;
    };

    bool operator <= ( const Set& M ) const { return containedIn( M ); }

    struct Sentinel{};

    struct Iter {

      const Set& S;
      unsigned i;

      Iter( const Set& X ): S{ X }, i{ 0 }{
        if( S.data[i].key == NIHIL or S.data[i].del ) ++(*this);
      }

      bool operator != ( [[maybe_unused]]const Sentinel& S ) const { return i < SPACE; }

      Iter& operator++ (){
        for(;;){
          i++;
          if( i >= SPACE                              ) return *this;
          if( S.data[i].key == NIHIL or S.data[i].del ) continue;
          return *this;
        }
      }

      Elem operator* (){ return S.data[i].key; }

    };//struct Iter

    auto begin() const { return Iter( *this ); }
    auto end  () const { return Sentinel();    }

	  double averageProbeCount() const {
	    unsigned num{ 0   };
		  double   sum{ 0.0 };
		  for( unsigned i = 0; i < SPACE; ++i ){
			  unsigned dib = data[i].dib;
		  	if( dib > 0 ){ num++, sum += dib;	}
	  	}
		  return num ? sum/num : 0.0;
	  }

  };//class Set
                                                                                                                              /*
  ______________________________________________________________________________________________________________________________
                                                                                                                              */

  template< typename Val, unsigned CAPACITY, unsigned LOAD_FACTOR_PERCENT = 80 > class Map {

    static constexpr unsigned SPACE = CAPACITY*100/LOAD_FACTOR_PERCENT;

    static_assert( std::is_trivially_copyable< Key >::value );
    static_assert( std::is_trivially_copyable< Val >::value );

  public:

    struct Entry {
      Key     key : 24;
      uint8_t dib :  7;
      bool    del :  1;
      Val     val;
    };

    enum Note: int8_t {
                                                                                                                              /*
      Explanation about incl/excl operation;
      Zero value means error, any other value means success
                                                                                                                              */
      EXHAUSTED = 0, // :not inserted because capacity exceeded
      INCLUDED  = 1, // :OK, included
      EXCLUDED  = 2, // :OK, excluded
      RECOVERED = 3, // :inserted element that was deleted earlier
      CONTAINED = 4, // :not inserted because already presented
      NOT_FOUND = 5, // :not excluded because not presented
      EMPTY_SET = 6  // :not excluded/included because empty set
    };

    static const char* lex( Note note ){
      switch( note ){
        case EXHAUSTED : return "EXHAUSTED";
        case INCLUDED  : return "INCLUDED ";
        case EXCLUDED  : return "EXCLUDED ";
        case RECOVERED : return "RECOVERED";
        case CONTAINED : return "CONTAINED";
        case NOT_FOUND : return "NOT_FOUND";
        case EMPTY_SET : return "EMPTY_SET";
        default        : break;
      }
      return nullptr;
    }

    static unsigned capacity(){ return CAPACITY;                                                      }
    static unsigned memory  (){ return sizeof( Entry )*SPACE + sizeof( cardinal ) + sizeof( Entry* ); }

  private:

	  uint32_t cardinal;
	  Entry*   data;

	  Note incl_( Key key, const Val& val, unsigned depth = 0 ){
      assert( key != NIHIL );
	    assert( key < UINT24 );
	    if( cardinal >= CAPACITY ) return EXHAUSTED;
      unsigned i{ key % SPACE                        }; // :desired position
      Entry    e{ key:key, dib:0, del:false, val:val }; // :DIB (distance to initial bucket) is zero initially
      for( unsigned c = i; ; c = ( c + 1 ) % SPACE ){
		    if( data[c].key == 0 ){ // Vacant cell, insert here
			    data[c] = e;
			    cardinal++;
			    return INCLUDED;
		    }
		    if( data[c].key == e.key ){ // Presented or deleted
			    if( data[c].del ){ // Sometime was presented but deleted - just recovery:
			      data[c].del = false;
			      data[c].val = e.val;
			      cardinal++;
			      return RECOVERED;
			    } else { // Presented:
			      data[c].val = e.val;
			      return CONTAINED;
			    }
			    break;
		    }
			  if( data[c].dib < e.dib ){ // To be swapped because it is rich.
			                                                                                                                        /*
			    Swap `e` and `data[c]`, i.e. insert here but move current element to some another place
			                                                                                                                        */
          std::swap( e, data[c] );
        }//if
        constexpr unsigned DIB_LIMIT{ 8 }; // :rehashing criterion
        if( e.dib >= DIB_LIMIT ){ return rehash( e, depth ); }
		    else e.dib++; //:the entry to be inserted goes away from ideal position gradually
      }// for c
		  return INCLUDED;
	  }//incl

    Note rehash( Entry e, unsigned depth ){
                                                                                                                              /*
      Collect all currently presented elements into the temporary array E:
                                                                                                                              */
      std::vector< Entry > E( cardinal );
      unsigned n{ 0 };
      for( unsigned i = 0; i < SPACE; i++ ) if( data[i].key != 0 and not data[i].del ) E[n++] = data[i];
      clear();
      for( unsigned i = 0; i < n; i++ ) assert( incl_( E[i].key, E[i].val, depth + 1 ) );
                                                                                                                              /*
      Finally add `elem`:
                                                                                                                              */
      if( e.key and not e.del ) assert( incl_( e.key, e.val, depth + 1 ) == INCLUDED );
      return INCLUDED;
    }

  public:

	  std::string content() const {
	    std::stringstream out;
	    out << std::setw( 4 ) << cardinal << " {";
	    for( unsigned i = 0; i < SPACE; i++ ){
	      Entry& e{ data[i] };
	      if( e.key == 0 ) out << "  empty ";
	      else {
	        if( e.del ) out << " ("; else out << "  ";
	        out << std::setw( 3 ) << e.key;
	        if( e.del ) out << ")"; else out << "`";
	        out << unsigned( e.dib );
	      }
	    }
	    out << " }";
	    return out.str();
	  }//content

    void clear(){
      cardinal = 0;
	    memset( data, 0, sizeof( Entry )*SPACE );
    }

	  Map(): cardinal{ 0 }, data{ new Entry[ SPACE ] }{
	    assert( data );
	    memset( data, 0, sizeof( Entry )*SPACE );
	  }

	  Map( const Map& ) = delete;
	  Map& operator = ( const Map& ) = delete;

	 ~Map(){ delete[] data; }

    bool vacant( Key key, unsigned maxDistance = 0 ) const {
      assert( key != NIHIL );
	    assert( key < UINT24 );
 	    if( cardinal >= CAPACITY ) return false;
      const unsigned desiredPosition{ key % SPACE }; // :desired position
      unsigned distance{ 0 };
      unsigned i{ desiredPosition };
	    while( not( data[i].key == NIHIL or data[i].del ) ){
		    i = ( i + 1 ) % SPACE;
		    if( ++distance > maxDistance ) return false; // :too distant
		    if( i == desiredPosition     ) return false; // :no vacant found in full loop
      }
      return true;
    }

	  Note incl( Key elem, const Val& val = Val{} ){ return incl_( elem, val ); }

    Val* get( const Key key ){
	    assert( key < UINT24 );
	    if( cardinal == 0  ) return nullptr;
      unsigned i{ key % SPACE }; // :desired position
	    while( data[i].key != key ){
		    i = ( i + 1 ) % SPACE;
		    if( i == key % SPACE ) return nullptr;	// :full loop performed but not found
      }
      assert( i < SPACE );
                                                                                                                              /*
      Key found, but it may be deleted, so result depends on date[.].del:
                                                                                                                              */
	    return data[i].del ? nullptr : &( data[i].val );
    }

    const Val* get( const Key key ) const {
	    assert( key < UINT24 );
	    if( cardinal == 0  ) return nullptr;
      unsigned i{ key % SPACE }; // :desired position
	    while( data[i].key != key ){
		    i = ( i + 1 ) % SPACE;
		    if( i == key % SPACE ) return nullptr;	// :full loop performed but not found
      }
      assert( i < SPACE );
                                                                                                                              /*
      Key found, but it may be deleted, so result depends on date[.].del:
                                                                                                                              */
	    return data[i].del ? nullptr : &( data[i].val );
    }

    bool contains( const Elem elem ) const {
	    assert( elem != NIHIL );
	    assert( elem < UINT24 );
	    if( cardinal == 0  ) return false;
      unsigned i{ elem % SPACE };
	    while( data[i].key != elem ){
		    i = ( i + 1 ) % SPACE;
		    if( i == elem % SPACE ) return false;	// :not Found, vacant call reached
      }
                                                                                                                              /*
      Key found, but it may be deleted, so result depends on date[.].del:
                                                                                                                              */
	    return not data[i].del;
    }

    Note excl( const Key elem ){
	    assert( elem < UINT24 );
	    if( elem == NIHIL ) return NOT_FOUND;
	    if( cardinal == 0 ) return EMPTY_SET;
      unsigned i{ elem % SPACE };
	    while( data[i].key != elem ){
		    i = ( i + 1 ) % SPACE;
		    if( i == elem % SPACE ){
		      return NOT_FOUND;	// :not Found
		    }
      }
                                                                                                                              /*
      Key found, mark it as deleted:
                                                                                                                              */
      if( not data[i].del ){
        data[i].del = true;
        cardinal--;
        return EXCLUDED;
      }
      return NOT_FOUND; // :the item has already been deleted earlier
    }

	  explicit operator bool() const{ return cardinal > 0; }

    const Val* operator[] ( const Key e ) const { return get( e ); }
          Val* operator[] ( const Key e )       { return get( e ); }

	  unsigned size () const { return cardinal;      }
	  bool     empty() const { return cardinal == 0; }

    struct Sentinel{};

    struct Iter {

      const Map& S;
      unsigned   i;

      Iter( const Map& X ): S{ X }, i{ 0 }{
        if( S.data[i].key == NIHIL or S.data[i].del ) ++(*this);
      }

      bool operator != ( const Sentinel& ) const { return i < SPACE; }

      Iter& operator++ (){
        for(;;){
          i++;
          if( i >= SPACE                              ) return *this;
          if( S.data[i].key == NIHIL or S.data[i].del ) continue;
          return *this;
        }
      }

      const Entry& operator* (){ return S.data[i]; }

    };//struct Iter

    auto begin() const { return Iter( *this ); }
    auto end  () const { return Sentinel();    }

	  double averageProbeCount() const {
	    unsigned num{ 0   };
		  double   sum{ 0.0 };
		  for( unsigned i = 0; i < SPACE; ++i ){
			  unsigned dib = data[i].dib;
		  	if( dib > 0 ){ num++, sum += dib;	}
	  	}
		  return num ? sum/num : 0.0;
	  }

  };//class Map


}//namespace CoreAGI::Flat

#endif // FLAT_H_INCLUDED

#pragma GCC diagnostic pop
