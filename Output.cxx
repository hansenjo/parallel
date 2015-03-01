// Output definition module for simple analyzer

#include "Output.h"
#include "Variable.h"

using namespace std;

const string& PlainVariable::GetName() const
{
  static const string nullstr("");
  if( !fVar ) return nullstr;
  return fVar->GetName();
}


ostrm_t& PlainVariable::write( ostrm_t& os, bool headerinfo ) const
{
  if( headerinfo ) {
    os.write( GetName().c_str(), GetName().size()+1 );
  } else {
    double x = fVar->GetValue();
    os.write( reinterpret_cast<const char*>(&x), sizeof(x) );
  }
  return os;
}

const string EventNumberVariable::fName = "Event";

ostrm_t& EventNumberVariable::write( ostrm_t& os, bool headerinfo ) const
{
  if( headerinfo ) {
    os.write( GetName().c_str(), GetName().size()+1 );
  } else {
    os.write( reinterpret_cast<const char*>(&fNev), sizeof(fNev) );
  }
  return os;
}

#if 0
void Output::Print() const
{
  for( vvec_t::const_iterator it = vars.begin(); it != vars.end(); ++it ) {
    (*it)->Print();
  }
}
#endif
