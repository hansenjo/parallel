// Output definition module for simple analyzer

#include "Output.h"
#include "Variable.h"

using namespace std;

PlainVariable::PlainVariable( Variable* var ) : fVar(var)
{
}

const string& PlainVariable::GetName() const
{
  static const string nullstr("");
  if( !fVar ) return nullstr;
  return fVar->GetName();
}


ostrm_t& PlainVariable::write( ostrm_t& os ) const
{
  os << fVar->GetValue();
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
