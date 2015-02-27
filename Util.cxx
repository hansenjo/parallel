// Utility functions

#include "Util.h"
#include <cstdio>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>

using namespace std;

// Count number of available CPUs. Requires glibc >= 2.6
int GetCPUcount()
{
  cpu_set_t cs;
  CPU_ZERO(&cs);
  sched_getaffinity(0, sizeof(cs), &cs);
  return CPU_COUNT(&cs);
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
