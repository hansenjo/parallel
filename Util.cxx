// Utility functions

#include "Util.h"
#include <thread>
#include <random>

#include <boost/tokenizer.hpp>

using namespace std;

static random_device rd;

// Count number of available threads
unsigned int GetThreadCount()
{
  return std::thread::hardware_concurrency();
}

// Test string 'candidate' against the wildcard expression 'expr'. Only "*" is
// supported as a wildcard character; it matches zero or more characters.
// Example: "h*ello*ld*" matches "hello world", "hello_molds" etc.
// Comparisons are case-sensitive. Return true if there is a match.
bool WildcardMatch( const string& candidate, const string& expr )
{

  using tokenizer = boost::tokenizer<boost::char_separator<char>>;

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

// Thread-safe random number generator
int intRand( int min, int max )
{
  thread_local minstd_rand generator(rd());
  uniform_int_distribution<int> distribution(min, max);
  return distribution(generator);
}
