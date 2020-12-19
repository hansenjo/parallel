// Helper routines for handling the per-thread analysis context

#include "Context.h"
#include "Detector.h"
#include "Variable.h"
#include "Util.h"

#include <iostream>
#include <fstream>
#include <boost/algorithm/string/trim.hpp>

using namespace std;
using namespace boost::algorithm;

Context::Context()
  : is_init(false), is_active(false)
{
  variables = make_shared<varlst_t>();
}

Context::Context( const Context& )
  : is_init(false), is_active(false)
{
}

Context::~Context()
{
  assert( !is_active );
}

int Context::Init()
{
  // Initialize current context

  // Initialize detectors
  int err = 0;
  for( auto& det : detectors ) {
    // Link each of our detectors to our variable list
    det->SetVarList(variables);
    // Init() calls the detector's ReadDatabase and DefineVariables
    if( int status = det->Init(false); status != 0 ) {
      err = status;
      cerr << "Error initializing detector " << det->GetName() << endl;
    }
  }
  if( err )
    return 1;

  // Read output definitions & configure output
  outvars.push_back( make_unique<EventNumberVariable>(nev) );

  if( cfg.odef_file.empty() )
    return 2;

  ifstream inp(cfg.odef_file);
  if( !inp ) {
    cerr << "Error opening output definition file " << cfg.odef_file << endl;
    return 2;
  }
  string line;
  while( getline(inp,line) ) {
    // Wildcard match variable names, ignoring trailing comments
    if( string::size_type pos = line.find('#'); pos != string::npos )
      line.erase(pos);
    trim(line);
    if( line.empty() )
      continue;
    for( auto& var : *variables ) {
      if( WildcardMatch(var->GetName(), line) )
	outvars.push_back( make_unique<PlainVariable>(var.get()) );
    }
  }
  line.clear();
  inp.close();

  if( outvars.empty() ) {
    // Noting to do
    cerr << "No output variables defined. Check " << cfg.odef_file << endl;
    return 3;
  }

  if( !evbuffer )
    evbuffer = make_unique<evbuf_t[]>(MAX_EVTSIZE);
  evbuffer[0] = 0;

  is_init = true;
  return 0;
}

#ifdef EVTORDER
int Context::fgNactive = 0;
std::mutex Context::fgMutex;
std::condition_variable Context::fgAllDone;

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
  evdata.Preload( evbuffer.get() );
  return evdata.IsSyncEvent();
}
#endif