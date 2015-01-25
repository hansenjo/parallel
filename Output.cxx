// Output definition module for simple analyzer

#include "Podd.h"
#include "Output.h"
#include "Variable.h"
#include <iostream>
#include <fstream>
#include <string>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace std;
using namespace boost::iostreams;

typedef vector<Variable*> vvec_t;

static const char* field_sep = ", ";

static bool WildcardMatch( string candidate, const string& expr )
{
  // Test string 'candidate' against the wildcard expression 'expr'.
  // Comparisons are case-insensitive
  // Return true if there is a match.

  typedef boost::tokenizer<boost::char_separator<char> >  tokenizer;

  boost::char_separator<char> sep("*");
  tokenizer tokens( expr, sep );
  string::size_type pos = 0;
  tokenizer::iterator tok = tokens.begin();
  while( tok != tokens.end() &&
         pos < candidate.length() &&
         (pos = candidate.find(*tok,pos)) != string::npos )
    {
      pos += (*tok).size();
      ++tok;
    }
  return (tok == tokens.end());
}

Output::Output()
{
}

int Output::Close()
{
  outs.reset();
  outp.close();
  vars.clear();
  return 0;
}

int Output::Init( const char* odat_file, const char* odef_file,
		  const varlst_t& varlst )
{
  if( !odat_file || !*odat_file || !odef_file || !*odef_file )
    return 1;

  vars.clear();

  // Read output definitions
  ifstream inp(odef_file);
  if( !inp ) {
    cerr << "Error opening output definition file " << odef_file << endl;
    return 2;
  }
  string line;
  while( getline(inp,line) ) {
    // Wildcard match
    string::size_type pos = line.find_first_not_of(" \t");
    if( line.empty() || pos == string::npos || line[pos] == '#' )
      continue;
    line.erase(0,pos);
    pos = line.find_first_of(" \t");
    if( pos != string::npos )
      line.erase(pos);
    for( varlst_t::const_iterator it = varlst.begin(); it != varlst.end(); ++it ) {
      Variable* var = *it;
      if( WildcardMatch(var->GetName(), line) )
	vars.push_back(var);
    }
  }
  inp.close();

  if( vars.empty() ) {
    // Noting to do
    cerr << "No output variables defined. Check " << odef_file << endl;
    return 3;
  }

  // Open output file and set up filter chain
  outs.reset();
  outp.close();
  outp.open( odat_file, ios::out|ios::trunc|ios::binary);
  if( !outp ) {
    cerr << "Error opening output data file " << odat_file << endl;
    return 4;
  }
  if( compress_output > 0 )
    outs.push(gzip_compressor());
  outs.push(outp);

  outs << "Event" << field_sep;
  for( vvec_t::iterator it = vars.begin(); it != vars.end(); ) {
    Variable* var = *it;
    outs << var->GetName();
    ++it;
    if( it != vars.end() )
      outs << field_sep;
  }
  outs << endl;

  return 0;
}

int Output::Process( int iev )
{
  if( !outp.is_open() || !outp.good() || !outs.good() )
    return 1;

  outs << iev;
  if( !vars.empty() )
    outs << field_sep;
  for( vvec_t::iterator it = vars.begin(); it != vars.end(); ) {
    Variable* var = *it;
    outs << var->GetValue();
    ++it;
    if( it != vars.end() )
      outs << field_sep;
  }
  outs << endl;

  return 0;
}

void Output::Print() const
{
  for( vvec_t::const_iterator it = vars.begin(); it != vars.end(); ++it ) {
    (*it)->Print();
  }
}
