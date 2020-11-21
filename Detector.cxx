// Simple detector base class

#include "Podd.h"
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

void Detector::Clear()
{
  data.clear();
}

int Detector::Init()
{
  return DefineVariables( kDefine );
}

int Detector::Decode( Decoder& evdata )
{
  // Generic detector decoding

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

int Detector::DefineVariables( bool )
{
  return 0;
}

// Declared in Podd.h
int DefineVarsFromList( const std::vector<VarDef_t>& defs,
                        const std::string& prefix,
                        varlst_t& varlst, bool remove )
{
  if( defs.empty() or (remove && varlst.empty()) )
    return 0;

  int ndef = 0;
  for( auto& def : defs ) {
    string varname(prefix);
    if( !prefix.empty() ) {
      varname.append(".");
    }
    varname.append(def.name);
    auto it = find_if( ALL(varlst), [&varname](auto& var) {
      assert(var != nullptr); return (var->GetName() == varname);
    });
    if( remove ) {
      if( it != varlst.end() ) {
	varlst.erase(it);
	++ndef;
      }
    } else {
      if( it != varlst.end() ) {
	cerr << "Variable " << varname << " already exists, skipped" << endl;
      } else if( !def.loc ) {
	cerr << "Invalid location pointer for variable " << varname
	     << ", skipped " << endl;
      } else {
	varlst.emplace_back( new Variable(varname, def.note, def.loc) );
	++ndef;
      }
    }
  }

  return ndef;
}
