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

Context::Context( int _id )
  : id(_id), is_init(false), is_active(false)
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
    // Init(shared = false) calls DefineVariables of the Detector
    if( int status = det->Init(false); status != 0 ) {
      err = status;
      cerr << "Error initializing detector " << det->GetName() << endl;
    }
  }
  if( err )
    return 1;

  // Read output definitions & configure output
  outvars.push_back( make_unique<EventNumberVariable>(nev) );

  string ofname = cfg.odef_file;
  if( ofname.empty() )
    return 2;

  ifstream inp(ofname);
  if( !inp ) {
    cerr << "Error opening output definition file " << ofname << endl;
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
    cerr << "No output variables defined. Check " << ofname << endl;
    return 3;
  }

#ifndef PPODD_TBB
  if( !evbuffer )
    evbuffer = make_unique<evbuf_t[]>(MAX_EVTSIZE);
  evbuffer[0] = 0;
#endif

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
