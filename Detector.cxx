// Simple detector base class

#include "Detector.h"
#include "Decoder.h"
#include "Variable.h"
#include <iostream>

using namespace std;

Detector::Detector( string _name, int _imod )
  : name(move(_name)), type("--baseclass--"), imod(_imod-1), fVars(nullptr)
{
  if( imod<0 ) {
    cerr << "\"" << name << "\": "
	 << "Warning: invalid module number = " << _imod << endl;
  }
}

// Clear event-by-event data
void Detector::Clear()
{
  data.clear();
}

// Initialize detector
// If 'shared' is true, initialize data shared among all instances
int Detector::Init( bool shared )
{
  int ret = ReadDatabase(shared);
  if( !shared )
    DefineVariables( kDefine );
  return ret;
}

// Generic detector decoding
int Detector::Decode( Decoder& evdata )
{
  int ndata = evdata.GetNdata(imod);
  if( debug > 1 )
    Print();
  if( debug > 2 )
    cout << " Decode: ndata = " << ndata;
  if( ndata > 0 ) {
    if( debug > 3 )
      cout << ", data = ";

    double* pdata = evdata.GetDataBuf(imod);
    data.assign( pdata, pdata+ndata );

    if( debug > 3 ) {
      for( int i = 0; i < ndata; ++i ) {
	cout << evdata.GetData(imod,i);
	if( i+1 != ndata )
	  cout << ", ";
      }
    }
  }
  if( debug > 2 )
    cout << endl;

  return 0;
}

void Detector::Print() const
{
  cout << "DET(" << type << "): " << name << endl;
}

// Define variables. The default implementation does nothing.
int Detector::DefineVariables( bool /* remove */ )
{
  return 0;
}

// Read database. The default implementation does nothing.
int Detector::ReadDatabase( bool /* shared */ ) {
  return 0;
}

// Declared in Podd.h
int DefineVarsFromList( const std::vector<VarDef_t>& defs,
                        const std::string& prefix,
                        varlst_t* varlst, bool remove )
{
  if( defs.empty() or (remove and (not varlst or varlst->empty())) )
    return 0;
  if( not varlst ) {
    cerr << "Missing variable list. Cannot define any variables" << endl;
    return 0;
  }
  auto& vars = *varlst;

  int ndef = 0;
  for( const auto& def : defs ) {
    string varname(prefix);
    if( !prefix.empty() ) {
      varname += '.';
    }
    varname.append(def.name);
    auto it = find_if( ALL(vars), [&varname](auto& var) {
      return (var && var->GetName() == varname);
    });
    if( remove ) {
      if( it != vars.end() ) {
	vars.erase(it);
	++ndef;
      }
    } else {
      if( it != vars.end() ) {
	cerr << "Variable " << varname << " already exists, skipped" << endl;
      } else if( !def.loc ) {
	cerr << "Invalid location pointer for variable " << varname
	     << ", skipped " << endl;
      } else {
	vars.push_back( make_unique<Variable>(varname, def.note, def.loc) );
	++ndef;
      }
    }
  }

  return ndef;
}
