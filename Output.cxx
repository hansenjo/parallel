// Output definition module for simple analyzer

#include "Podd.h"
#include "Output.h"
#include "Variable.h"
#include <iostream>
#include <fstream>
#include <string>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

PlainVariable::PlainVariable( Variable* var ) : fVar(var)
{
}

ostrm_t& PlainVariable::write( ostrm_t& os ) const
{
  os << fVar->GetValue();
  return os;
}


#if 0
typedef vector<Variable*> vvec_t;

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

int Output::Init( const char* odef_file, const varlst_t& varlst )
{
  if( !odef_file || !*odef_file )
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

  return 0;
}


void Output::Print() const
{
  for( vvec_t::const_iterator it = vars.begin(); it != vars.end(); ++it ) {
    (*it)->Print();
  }
}
#endif
