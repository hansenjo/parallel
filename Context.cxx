// Helper routines for handling the per-thread analysis context

#include "Context.h"
#include "Detector.h"
#include "Variable.h"
#include "Util.h"

#include <iostream>
#include <fstream>

using namespace std;

int Context::fgNactive = 0;
std::mutex Context::fgMutex;
std::condition_variable Context::fgAllDone;

Context::Context()
  : evbuffer(nullptr), is_init(false), is_active(false)
{

}

Context::Context( const Context& )
  : evbuffer(nullptr), is_init(false), is_active(false)
{
}

Context::~Context()
{
  assert( !is_active );
  DeleteContainer( outvars );
  DeleteContainer( detectors );
  DeleteContainer( variables );
  delete [] evbuffer;
}

int Context::Init( const char* odef_file )
{
  // Initialize current context

  DeleteContainer(outvars);
  DeleteContainer(variables);
  delete [] evbuffer; evbuffer = nullptr;

  // Initialize detectors
  int err = 0;
  for(auto* det : detectors) {
    int status;
    det->SetVarList(variables);
    if( (status = det->Init()) != 0 ) {
      err = status;
      cerr << "Error initializing detector " << det->GetName() << endl;
    }
  }
  if( err )
    return 1;

  // Read output definitions & configure output
  outvars.push_back( new EventNumberVariable(nev) );

  if( !odef_file || !*odef_file )
    return 2;

  ifstream inp(odef_file);
  if( !inp ) {
    cerr << "Error opening output definition file " << odef_file << endl;
    return 2;
  }
  string line;
  while( getline(inp,line) ) {
    // Wildcard match
    string::size_type pos = line.find_first_not_of(" \t");
    if( line.empty() || pos == string::npos || line[pos] == '#' )
      continue;
    line.erase(0,pos);
    pos = line.find_first_of(" \t");
    if( pos != string::npos )
      line.erase(pos);
    for( auto *var : variables ) {
      if( WildcardMatch(var->GetName(), line) )
	outvars.push_back( new PlainVariable(var) );
    }
  }
  inp.close();

  if( outvars.empty() ) {
    // Noting to do
    cerr << "No output variables defined. Check " << odef_file << endl;
    return 3;
  }

  evbuffer = new evbuf_t[INIT_EVSIZE];
  evbuffer[0] = 0;

  is_init = true;
  return 0;
}

void Context::MarkActive()
{
  is_active = true;
  std::lock_guard lock(fgMutex);
  ++fgNactive;
}

void Context::UnmarkActive()
{
  is_active = false;
  {
    std::lock_guard lock(fgMutex);
    --fgNactive;
    assert( fgNactive >= 0 );
  }
  if( fgNactive == 0 )
    fgAllDone.notify_one();
}

void Context::WaitAllDone()
{
  std::unique_lock lock(fgMutex);
  while( fgNactive > 0 )
    fgAllDone.wait(lock);
}

bool Context::IsSyncEvent()
{
  evdata.Preload( evbuffer );
  return evdata.IsSyncEvent();
}
