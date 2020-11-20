// Simple detector base class

#include "Podd.h"
#include "Detector.h"
#include "Decoder.h"
#include "Variable.h"
#include <iostream>

using namespace std;

Detector::Detector( const char* _name, int _imod )
  : name(_name), type("--baseclass--"), imod(_imod-1), fVars(nullptr)
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
int DefineVarsFromList( VarDef_t* defs, const char* prefix,
			varlst_t* varlst, bool remove )
{
  if( !defs || !varlst )
    return 0;

  int ndef = 0;
  VarDef_t* def = defs;
  while( def->name ) {
    string varname;
    if( prefix && *prefix ) {
      varname.append(prefix);
      varname.append(".");
    }
    varname.append(def->name);
    auto it = varlst->begin();
    for( ; it != varlst->end(); ++it ) {
      if( (*it)->GetName() == varname ) {
	break;
      }
    }
    if( remove ) {
      if( it != varlst->end() ) {
	varlst->erase(it);
	++ndef;
      }
    } else {
      if( it != varlst->end() ) {
	cerr << "Variable " << varname << " already exists"
	  ", skipped" << endl;
      } else if( !def->loc ) {
	cerr << "Invalid location pointer for variable " << varname
	     << ", skipped " << endl;
      } else {
	varlst->emplace_back( new Variable(varname.c_str(), def->note, def->loc) );
	++ndef;
      }
    }
    ++def;
  }

  return ndef;
}
