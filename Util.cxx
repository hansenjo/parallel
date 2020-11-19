// Utility functions

#include "Util.h"
#include <thread>

#include <boost/tokenizer.hpp>


using namespace std;

// Count number of available threads
unsigned int GetThreadCount()
{
  return std::thread::hardware_concurrency();
}

bool WildcardMatch( const string& candidate, const string& expr )
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
