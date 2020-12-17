// Simple detector base class

#include "Detector.h"
#include "Decoder.h"
#include "Variable.h"
#include <fstream>
#include <iostream>
#include <cassert>
#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>

using namespace std;
using namespace boost;
using namespace boost::algorithm;

class bad_db_syntax : public std::runtime_error {
public:
  explicit bad_db_syntax( const std::string& what_arg )
          : std::runtime_error(what_arg) {}
};

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

int Detector::DefineVariables( bool /* remove */ )
{
  return 0;
}

// Read database file, if it exists.
int Detector::ReadDatabase( bool /* shared */ )
{
  ifstream ifs(cfg.db_file);
  if( ifs ) {
    string line;
    while( getline(ifs, line) ) {
      // Blank line or comment?
      if( string::size_type pos = line.find('#'); pos != string::npos )
        line.erase(pos);
      trim(line);
      if( line.empty() )
        continue;

      // Extract key/value pairs. Each non-comment line in the database file
      // should have the format
      //   module.key = value
      // where "value" must be convertible to double. Trailing garbage
      // results in an error.
      try {
        ParseDBline(line);
      }
      catch( const bad_db_syntax& e ) {
        cerr << "Bad database syntax: " << e.what() << endl;
        return 1;
      }
      catch( const std::exception& e ) {
        cerr << "Bad database line: " << line << ": " << e.what() << endl;
        return 1;
      }
    }
  }
  return 0;
}

// Parse the full key into module.key parts.
// Returns the number of components found between dots.
int Detector::ParseDBkey( const string& fullkey, DBitem& item )
{
  char_separator<char> dot(".");
  tokenizer<char_separator<char>> parts(fullkey, dot);
  int j = 0;
  for( const auto& p : parts ) {
    if( j == 0 ) {
      item.module = p;
    } else if( j == 1 ) {
      item.key = p;
    } else if( j > 1 ) {
      // The key itself may contain dots
      item.key += ".";
      item.key.append(p);
    }
    ++j;
  }
  // Put a component without a module prefix into the key field
  // instead the module field. Example:
  //   "debug = 1" -> module = "", key = "debug", value = "1.0"
  if( j == 1 ) {
    assert(item.key.empty());  // Else logic error in parser above
    item.key.swap(item.module);
  }
  return j;
}

// Parse database line and put results in member variable 'dbitems'
void Detector::ParseDBline( const string& line )
{
  DBitem item;
  char_separator<char> whitespace(" \t");
  tokenizer<char_separator<char>> tokens(line, whitespace);
  int i = 0;
  for( const auto& t : tokens ) {
    if( i == 0 ) {
      if( t == "=" )
        throw bad_db_syntax(line);
      // Parse module.key part
      if( ParseDBkey(t,item) == 0 )
        throw bad_db_syntax(line);
    } else if( i == 1 ) {
      if( t != "=" )
        throw bad_db_syntax(line);
    } else if( i == 2 ) {
      item.value = stod(t);
    }
    ++i;
  }
  if( i != 3 )
    throw bad_db_syntax(line);

  dbitems.push_back(std::move(item));
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
