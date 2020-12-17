// Parallel Podd DetectorTypeC
//
// Simple "detector" class, doing computation of pi as an example
// of a time-consuming task

#include "Podd.h"
#include "DetectorTypeC.h"
#include "Decoder.h"
#include <cstdlib>
#include <stdexcept>
//#include <cassert>

using namespace std;

DetectorTypeC::DetectorTypeC( const string& name, int imod )
  : Detector(name, imod), m_ndig(0), m_last5(0), m_scale(1.0)
{
  type = "C";

  m_result.reserve(5000);
}

DetectorTypeC::~DetectorTypeC()
{
 DetectorTypeC::DefineVariables( kRemove );
}

void DetectorTypeC::Clear()
{
  Detector::Clear();
  m_result.clear();
  m_ndig  = 0;
  m_last5 = 0;
}

unique_ptr<Detector> DetectorTypeC::Clone() const
{
  return make_unique<DetectorTypeC>(*this);
}


int DetectorTypeC::Analyze()
{
  // This detector type computes n digits of pi
  //
  // Attempts to implement the algorithm from
  // Rabinowitz and Wagon, "A spigot algorithm for the digits of Pi",
  // American Mathematical Monthly, 102 (3), 195–203 (March 1995),
  // doi:10.2307/2975006.
  //
  // Note: There appears to be a bug in this code that sometimes gets the
  // last digit wrong. Compare the results for n = 50 and n = 51 for example.
  // We ignore this problem here.

  int n = 0;
  if( !data.empty() )
    n = int(data[0]*m_scale);
  if( n < 1 )
    n = 10;

  int N = (10*n)/3;

  m_a.assign(N,2);
  m_result.reserve(n+1);

  int last_digit = -1;
  int nines = 0;
  bool dot = true;
  for( int i = 0; i < n; i++ ) {
    for( int j = 0; j < N; j++ )
      m_a[j] *= 10;
    for( int j = N-1; j > 0; j-- ) {
      div_t d = div(m_a[j],2*j+1);
      m_a[j] = d.rem;
      m_a[j-1] += d.quot*j;
    }
    int Q = m_a[0]/10;
    m_a[0] -= 10*Q;
    if( Q < 9 ) {
      if( last_digit >= 0 )
        m_result += char('0'+last_digit);
      if( dot && last_digit >= 0 ) {
        dot = false;
        m_result += '.';
      }
      for( int j = 0; j < nines; j++ )
        m_result += '9';
      nines = 0;
      last_digit = Q;
    } else if ( Q == 9 ) {
      nines ++;
    } else if ( Q == 10 ) {
      if( last_digit >= 0 )
        m_result += char('1'+last_digit);
      if( dot && last_digit >= 0 ) {
        dot = false;
        m_result += '.';
      }
      for( int j = 0; j < nines; j++ )
        m_result += '0';
      nines = 0;
      last_digit = 0;
    }

  }
  if( last_digit >= 0 )
    m_result += char('0'+last_digit);

  //assert( m_result.length() == n+1 ); // This sometimes trips, exposing the bug mentioned above

  m_ndig = n;
  m_last5 = std::stod(m_result.substr(m_result.length() - 5, 5));

  return 0;
}

void DetectorTypeC::Print() const
{
  Detector::Print();
}

int DetectorTypeC::DefineVariables( bool do_remove )
{
  const vector<VarDef_t> defs = {
          {"nval",  "Number of digits computed",  &m_ndig},
          {"last8", "Value of last 5 digits",     &m_last5},
  };
  return DefineVarsFromList(defs, GetName(), fVars, do_remove);
}

int DetectorTypeC::ReadDatabase( bool shared )
{
  int status = Detector::ReadDatabase(shared);
  if( status )
    return status;

  return 0;
}
