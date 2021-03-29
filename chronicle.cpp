                                                                                                                              /*
 Copyright Mykola Rabchevskiy 2021.
 Distributed under the Boost Software License, Version 1.0.
 (See http://www.boost.org/LICENSE_1_0.txt)
 ______________________________________________________________________________

 Test aplication for `Chronicle` module

 NOTE

   Source files MUST be ASCII-encoded.
   To convert Unicade to ASCII use `uni2ascii` convertor:

     uni2ascii [options] path-to-file


________________________________________________________________________________________________________________________________
                                                                                                                              */
#include <cassert>
#include <cstdio>

#include <filesystem>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <ranges>
#include <string>
#include <vector>

#include "arena.h"
#include "chronicle.h"
#include "def.h"
#include "random.h"         // :random pattern ID
#include "timer.h"

using namespace CoreAGI;

namespace fs = std::filesystem;


struct View{
                                                                                                                              /*
  If `tail` == 0 then pattern represents atomic entity ~ char
  and `head` value must be in [ 0..255 ]:
                                                                                                                              */
  Identity head;
  Identity tail;

  View(): head{ CoreAGI::NIHIL }, tail{ CoreAGI::NIHIL }{}

  View( const Identity& h, const Identity& t ): head{ h }, tail{ t }{
    assert( head != CoreAGI::NIHIL );
    assert( tail != CoreAGI::NIHIL );
  }

  View& operator= ( const View& P ){ head = P.head; tail = P.tail; return *this; }

  bool operator== ( const View& P ) const { return head == P.head and tail == P.tail; }

};//View


using Atom = char;


struct Pattern {

  std::span< Identity > seq;

  Pattern( std::span< Identity > seq ): seq{ seq }{}
  Pattern(                           ): seq{     }{}

  bool operator== ( const Pattern& P ) const {
    if( seq.size() != P.seq.size() ) return false;
    for( unsigned i = 0; i < seq.size(); i++ ) if( seq[i] != P.seq[i] ) return false;
    return true;
  }

};//struct Pattern

using Key = CoreAGI::Key;   // : see def.h

namespace std {
                                                                                                                              /*
  Hash function for View:
                                                                                                                              */
  template<> struct hash< View > {
    std::size_t operator()( const View& P ) const { return combination( P.head, P.tail ); }
  };

  template<> struct hash< Pattern > {
    std::size_t operator()( const Pattern& P ) const {
      std::size_t result{ 0 };
      for( const auto& elem: P.seq ) result ^= elem;
      return result;
    }
  };

}//namespace std


int main(){

  constexpr unsigned CAPACITY        { 512*1024 }; // :sequence capacity
  constexpr unsigned STORAGE_CAPACITY{ 256*1024 }; // :pattern storage bytes
  constexpr size_t   UINT24MASK      { 0xFFFFFF };
  constexpr char     CR              { '\r'     };
  constexpr char     SPC             { ' '      };
                                                                                                                              /*
  Simplistic semantic storage for patterns:
                                                                                                                              */
  Arena arena{ STORAGE_CAPACITY };                            // :storage for pattern` objects

  Identity ATOM[ 256 ];                                              // :symbol -> atom ID
  for( unsigned i = 0; i < 156; i++ ) ATOM[i] = CoreAGI::NIHIL;      // :make empty map
  std::unordered_map< Identity, Atom    > SYMBOL;                    // :atom ID -> symbol
  std::unordered_set< Identity          > unconnectable;             // :kind of atoms
  std::unordered_map< Identity, Pattern > PATTERN;                   // :pattern ID     -> Pattern` object
  std::unordered_map< View,     Identity> DICTIONARY;                // :pattern view   -> pattern` ID
  std::unordered_map< Pattern,  Identity> GLOSSARY;                  // :pattern object -> pattern` ID
                                                                                                                              /*
  Check if ID is presented:
                                                                                                                              */
  auto atomic    = [&]( const Identity& id )->bool{ return SYMBOL .find( id ) != SYMBOL .end(); };
  auto composite = [&]( const Identity& id )->bool{ return PATTERN.find( id ) != PATTERN.end(); };
  auto exist     = [&]( const Identity& id )->bool{ return composite( id ) or atomic( id );     };

  auto expo = [&]( const Identity* sequence, unsigned length ){
    printf( "%c", '`' );
    for( unsigned i = 0; i < length; i++ ){
      const Identity& element{ sequence[i] };
      if( SYMBOL.contains( element ) ) printf( "%c",   SYMBOL.at( element ) );
      else                             printf( "{%u}", element              );
    }
    printf( "%c", '`' );  fflush( stdout );
  };

  auto appendAtom = [&]( Identity* out, /*mod*/unsigned* lim, /*mod*/unsigned* len, const Identity& atom )->Identity*{
    assert( *lim > 1       );
    *out = atom;
    (*len)++;
    (*lim)--;
    return out + 1;
  };

  auto appendPattern = [&]( Identity* out, /*mod*/unsigned* lim, /*mod*/unsigned* len, const Pattern& pattern )->Identity*{
    assert( *lim > pattern.seq.size() );
    unsigned* loc = out;
    for( const auto& atom: pattern.seq ) loc = appendAtom( loc, lim, len, atom );
    return loc;
  };

  auto unfold = [&]( Identity* seq, /*mod*/unsigned* lim, /*mod*/unsigned* len, const Identity& id )->Identity*{
                                                                                                                              /*
    Unfold pattern sequence into `seq` or place atom ID into `seq`; returns number of added elements:
                                                                                                                              */
    if( atomic( id ) ) return appendAtom( seq, lim, len, id );
    assert( composite( id ) );
    return appendPattern( seq, lim, len, PATTERN.at( id ) );
  };
                                                                                                                              /*
  Generation unique ID:
                                                                                                                              */
  auto uniqueId = [&]()->Identity{
    Identity id{ CoreAGI::NIHIL };
    do{ id = Identity( randomNumber() & UINT24MASK ); } while( exist( id ) );
    return id;
  };

  auto atom = [&]( Atom symbol )->Identity {
    const unsigned i{ unsigned( symbol ) };
    assert( ATOM[ i ] == CoreAGI::NIHIL ); // :prevent Atom re-definition
    const Identity id{ uniqueId() };
    SYMBOL[ id ] = symbol;
    ATOM  [ i  ] = id;
    return id;
  };

  auto pattern = [&]( const Identity& head, const Identity& tail )->Identity {
    assert( head != CoreAGI::NIHIL );
    assert( tail != CoreAGI::NIHIL );
    assert( exist( head )          );
    assert( exist( tail )          );
                                                                                                                              /*
    Compose pattern sequence in `seq` array with actual length `len`:
                                                                                                                              */
    constexpr unsigned CAPACITY{ 256 };
    Identity  seq[ CAPACITY ];              // :pattern` sequence
    Identity* loc{ seq      };              // :pointer to first vacant position
    unsigned  lim{ CAPACITY };              // :how mant symbols can be added
    unsigned  len{ 0        };              // :pattern` length
    loc = unfold( loc, &lim, &len, head );
    loc = unfold( loc, &lim, &len, tail );
    assert( len >= 2 );
                                                                                                                              /*
    Make Pattern instance with content stored in the `seq`:
                                                                                                                              */
    Pattern P{ std::span< Identity >( seq, len ) };
                                                                                                                              /*
    Check if such pattern already presented:
                                                                                                                              */
    Identity id{ CoreAGI::NIHIL };
    const auto& it = GLOSSARY.find( P );
    if( it != GLOSSARY.end() ){
                                                                                                                              /*
      Such petter already presented, so make View only and store it:
                                                                                                                              */
      id = it->second;
      DICTIONARY[ View( head, tail ) ] = id;
    } else {
                                                                                                                              /*
      Make random pattern` ID:
                                                                                                                              */
      id = uniqueId();
                                                                                                                              /*
      Allocate pattern sequence:
                                                                                                                              */
      Pattern Q{ arena.span< Identity >( len ) };
      assert( Q.seq.size() == len );
                                                                                                                              /*
      Copy pattern sequence:
                                                                                                                              */
      memcpy( Q.seq.data(), seq, sizeof( Identity )*len );
                                                                                                                              /*
      Store pattern and view:
                                                                                                                              */
      DICTIONARY[ View( head, tail ) ] = id;
      GLOSSARY  [ Q                  ] = id;
      PATTERN   [ id                 ] = Q;
    }
    return id;
  };
                                                                                                                              /*
  Functions that provided external functionality for the Chronicle object:
                                                                                                                              */
  std::function< std::string( const Identity& ) > lex = [&]( const Identity& id )->std::string{
    if( atomic( id ) ){
      return std::string( 1, char( SYMBOL.at( id ) ) );
    }
    char P[ 256 ];
    unsigned i{ 0 };
    assert( PATTERN.contains( id ) );
    for( const Identity& elem: PATTERN.at( id ).seq ){
      P[i++] = SYMBOL.at( elem );
      if( i > 254 ) break;
    }
    P[i] = '\0';
    return std::string( P );
  };

  auto sticky = [&]( const Identity& head, const Identity& tail )->bool {
    if( unconnectable.contains( head )                    ) return false;
    if( unconnectable.contains( tail ) and atomic( head ) ) return false;
    return true;
  };

  auto find = [&]( const Identity& head, const Identity& tail )->Identity {
    const auto& it = DICTIONARY.find( View{ head, tail } );
    if( it != DICTIONARY.end() ) return it->second;
                                                                                                                              /*
    Compose pattern sequence:
                                                                                                                              */
    constexpr unsigned CAPACITY{ 256 };
    Identity  seq[ CAPACITY ];
    Identity* loc{ seq      };
    unsigned  lim{ CAPACITY };
    unsigned  len{ 0        };
    loc = unfold( loc, &lim, &len, head );
    loc = unfold( loc, &lim, &len, tail );
                                                                                                                              /*
    Make Pattern instance:
                                                                                                                              */
    Pattern P{ std::span< Identity >( seq, len ) };
                                                                                                                              /*
    Check if such pattern already presented:
                                                                                                                              */
    Identity id{ CoreAGI::NIHIL };
    const auto& itt = GLOSSARY.find( P );
    if( itt != GLOSSARY.end() ){
                                                                                                                              /*
      Such petter already presented, so make View only:
                                                                                                                              */
      id = itt->second;
      DICTIONARY[ View( head, tail ) ] = id;
    }
    return id;
  };
                                                                                                                              /*
  Set of special symbols:
                                                                                                                              */
  const Identity SPACE{ atom( ' ' ) };
  const Identity DOT  { atom( '.' ) };
  const Identity COLON{ atom( ':' ) };
  const Identity COMMA{ atom( ',' ) };
  const Identity EXCL { atom( '!' ) };
  const Identity QUEST{ atom( '?' ) };
  const Identity QUOT1{ atom( '\'') };
  const Identity QUOT2{ atom( '"' ) };

  unconnectable.insert( SPACE );
  unconnectable.insert( DOT   );
  unconnectable.insert( COLON );
  unconnectable.insert( COMMA );
  unconnectable.insert( EXCL  );
  unconnectable.insert( QUEST );
  unconnectable.insert( QUOT1 );
  unconnectable.insert( QUOT2 );

  Chronicle< CAPACITY > chronicle( lex, sticky, pattern, find );

  unsigned totalSymbolsProcessed{ 0   };
  double   dt                   { 0.0 };
  unsigned compno               { 0   };
  unsigned hasContinuation      { 0   };

  auto process = [&]( const char* path ){
    printf( "\n\n Process `%s`\n", path );
    fflush( stdout );
    FILE* src   { fopen( path, "r" ) };
    Atom  symbol{ 0                  };
    Atom  prev  { SPC                };
    Timer timer;
    for(;;){
      if( totalSymbolsProcessed % 10000 == 0 ){
        printf( "\r Processed %10u symbols", totalSymbolsProcessed );
        fflush( stdout );
      }
      if( ( symbol = (char)fgetc( src ) ) == EOF ) break;
      if( symbol == CR                                               ) continue;
      if( symbol >= 127 or symbol < 32 or not std::isprint( symbol ) ) symbol = SPC;
      if( symbol == SPC and prev == SPC                              ) continue;     // :eliminate multiple spaces
      symbol = tolower( symbol );                                                    // :convert symbol to lower case
      totalSymbolsProcessed++;
                                                                                                                              /*
      Get ID of the known Atom or make a new atom:
                                                                                                                              */
      Identity id = ATOM[ unsigned( symbol ) ];
      if( id == 0 ) id = atom( symbol );
      assert( chronicle.incl( id ) );
      if( not atomic( chronicle.lastId() ) ) hasContinuation++;
      prev = symbol;
      if( chronicle.gap() >= 16*1024 ){ chronicle.compact(); compno++; }
    }//forever
    timer.stop();
    dt += timer( Timer::MICROSEC );
    fclose( src );
    printf( "\r Processed %10u symbols", totalSymbolsProcessed );
    fflush( stdout );
  };

  std::vector< std::string > sources;
  constexpr const char* SOURCES{ "txt" };
  printf( "\n\n Sources from %s\n", SOURCES );
  for( auto& entry: fs::directory_iterator( SOURCES ) ){
    unsigned    fsize = entry.file_size();
    std::string fpath = entry.path().string();
    printf( "\n   %8u bytes  %s", fsize, fpath.c_str() );
    sources.push_back( fpath );
  }
  for( const auto& path: sources ) process( path.c_str() );
  const double contPerCent = 100.0*hasContinuation/totalSymbolsProcessed;
  const double compression = double( totalSymbolsProcessed )/chronicle.len();
  printf( "\n\n Total %u symbols processed in %.2f msec ~ %.2f microsec/symbol",
                totalSymbolsProcessed, dt, dt/totalSymbolsProcessed );
  double fraction{ double( chronicle.len()/double( CAPACITY ) ) };
  printf( "\n Sequence length                    %6u elements ~ %.2f %% of capacity", chronicle.len(), 100.0*fraction );
  printf( "\n Compacted                          %6u times",    compno                    );
  printf( "\n Gap                                %6u",          chronicle.gap()           );
  printf( "\n Total number of patterns           %6lu",         PATTERN.size()            );
  printf( "\n Total number of views              %6lu",         DICTIONARY.size()         );
  printf( "\n Distinct elements in the sequence  %6u",          chronicle.distinct()      );
  printf( "\n Cases witch continuations          %9.2f %%",     contPerCent               );
  printf( "\n Sequence compression ratio         %9.2f",        compression               );
  printf( "\n Arena memory allocated             %9.2f Kb",     0.001*arena.occupied()    );
  printf( "\n Arena memory available             %9.2f Kb",     0.001*arena.available()   );
  printf( "\n Sequence memory                    %9.2f Kb",     0.001*sizeof( chronicle ) );

  {                                                                                                                           /*
    Process patterns:
                                                                                                                              */
    constexpr const char* PATTERNS_PATH{ "patterns.txt" };
    constexpr unsigned    TOP          { 100            };
    std::vector< std::string > patterns;
    std::vector< std::string > top;
    unsigned maxLen{ 0     };
    bool     sorted{ false };

    struct{ bool operator()( std::string L, std::string R ) const { return L.size() > R.size(); } } comp;

    for( const auto& [ id, pattern ]: PATTERN ){
      const unsigned length = pattern.seq.size();
      if( length > maxLen ) maxLen = length;
      const std::string seq = lex( id );
      patterns.push_back( seq );
      if( top.size() < TOP ){
        top.push_back( seq );
      } else {
        if( not sorted ){
          std::ranges::sort( top, comp );
          sorted = true;
        }
        if( seq.size() > top.back().size() ){
          top.push_back( seq );
          std::ranges::sort( top, comp );
          sorted = true;
          while( top.size() > TOP ) top.pop_back();
        }
      }
    }
    std::ranges::sort( patterns );
    printf( "\n\n Max pattern` length: %u", maxLen );
    printf( "\n\n Top %u longest patterns:\n", TOP );
    for( int i=0; const auto p: top ) printf( "\n %3u `%s`", ++i, p.c_str() );
    printf( "\n\n Save patterns as %s..", PATTERNS_PATH );
    FILE* out = fopen( PATTERNS_PATH, "w" );
    for( int ord=0; const auto& p: patterns ) fprintf( out, " %05u  `%s`\n", ++ord, p.c_str() );
    fflush( out );
    fclose( out );
  }
  {                                                                                                                           /*
    Process continuations:
                                                                                                                              */
    std::unordered_map< Identity, std::unordered_set< Identity > > SEQUEL;
    for( const auto&[ view, id ]: DICTIONARY ) SEQUEL[ view.head ].insert( view.tail );
    std::vector< Identity > CONTEXT;
    for( const auto& [ context, X ]: SEQUEL ) CONTEXT.push_back( context );

    struct Comp{
      std::function< std::string( Identity ) > lex;
      bool operator()( const Identity& left, const Identity right ) const {
        const std::string L{ lex( left  ) };
        const std::string R{ lex( right ) };
        if( L.size() == R.size() ) return L < R;
        return L.size() > R.size();
      }
      Comp( std::function< std::string( Identity ) > lex ): lex( lex ){}
    };

    Comp comp( lex );
    std::ranges::sort( CONTEXT, comp );
    unsigned total{ 0 };
    constexpr unsigned HISTOGRAM_CAPACITY{ 1024 };
    unsigned HISTOGRAM[ HISTOGRAM_CAPACITY ];
    for( auto& Hi: HISTOGRAM ) Hi = 0;
    constexpr const char* SEQUEL_PATH{ "sequel.txt" };
    printf( "\n\n Save continuationsa as %s..", SEQUEL_PATH );
    FILE* out = fopen( SEQUEL_PATH, "w" );
    for( int ord=0; const auto& context: CONTEXT ){
      std::unordered_set< Identity >& M{ SEQUEL[ context ] };
      std::vector< Identity > sequel( M.begin(), M.end() );
      HISTOGRAM[ sequel.size() ]++, total++;
      std::ranges::sort( sequel, comp );
      fprintf( out, " %05u  `%s` \n", ++ord, lex( context ).c_str() );
      for( int i=0; const auto Ci: sequel ) fprintf( out, " %5u  `%s` \n", ++i, lex( Ci ).c_str() );
    }
    fflush( out );
    fclose( out );
    printf( "\n\n Distribution of the numbers of continuations:\n" );
    double sum{ 0.0 };
    for( unsigned len = 0; len < HISTOGRAM_CAPACITY; len++ ){
      unsigned num = HISTOGRAM[ len ];
      if( num == 0 ) continue;
      const double fraction{ double( num )/double( total ) };
      sum += fraction;
      printf( "\n %4u continuation: %6u times ~ %6.2f %%", len, num, 100.0*fraction );
    }
    printf( "\n\n sum: %.3f", sum ); // DEBUG
  }
  printf( "\n\n Finish\n\n" );
  return EXIT_SUCCESS;
}
