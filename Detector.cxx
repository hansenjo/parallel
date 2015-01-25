// Simple detector base class

#include "Detector.h"
#include <iostream>

using namespace std;

Detector::Detector( const char* _name ) : name(_name), type("abstract")
{
}

void Detector::Clear()
{
}

int Detector::Init()
{
  return DefineVariables( kDefine );
}

int Detector::Analyze()
{
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

int Detector::DefineVarsFromList( VarDef_t* defs, const char* prefix,
				  varlst_t& varlst, bool remove )
{
  if( !defs )
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
    varlst_t::iterator it = varlst.begin();
    for( ; it != varlst.end(); ++it ) {
      if( (*it)->GetName() == varname ) {
	break;
      }
    }
    if( remove ) {
      if( it != varlst.end() ) {
	delete *it;
	varlst.erase(it);
	++ndef;
      }
    } else {
      if( it != varlst.end() ) {
	cerr << "Variable " << varname << " already exists"
	  ", skipped" << endl;
      } else if( !def->loc ) {
	cerr << "Invalid location pointer for variable " << varname
	     << ", skipped " << endl;
      } else {
	varlst.push_back( new Variable(varname.c_str(), def->note, def->loc) );
	++ndef;
      }
    }
    ++def;
  }

  return ndef;
}
