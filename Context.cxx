// Helper routines for handling the per-thread analysis context

#include "Context.h"
#include "Detector.h"
#include "Variable.h"
#include "Util.h"

#include <iostream>
#include <fstream>

using namespace std;

int Context::fgNctx = 0;
int Context::fgNactive = 0;
pthread_mutex_t Context::fgMutex;
pthread_cond_t  Context::fgAllDone;

void Context::StaticInit()
{
  pthread_mutex_init( &fgMutex, 0 );
  pthread_cond_init( &fgAllDone, 0 );
}

void Context::StaticCleanup()
{
  pthread_mutex_destroy( &fgMutex );
  pthread_cond_destroy( &fgAllDone );
}

Context::Context() : evbuffer(0), is_init(false), is_active(false)
{
  if( fgNctx == 0 )
    StaticInit();
  ++fgNctx;
}

Context::Context( const Context& )
  : evbuffer(0), is_init(false), is_active(false)
{
  ++fgNctx;
}

Context::~Context()
{
  assert( !is_active );
  DeleteContainer( outvars );
  DeleteContainer( detectors );
  DeleteContainer( variables );
  delete [] evbuffer;
  --fgNctx;
  assert( fgNctx >= 0 );
  if( fgNctx == 0 )
    StaticCleanup();
}

int Context::Init( const char* odef_file )
{
  // Initialize current context

  DeleteContainer(outvars);
  DeleteContainer(variables);
  delete [] evbuffer; evbuffer = 0;

  // Initialize detectors
  int err = 0;
  for( detlst_t::iterator it = detectors.begin(); it != detectors.end(); ++it ) {
    int status;
    Detector* det = *it;
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
    for( varlst_t::const_iterator it = variables.begin();
	 it != variables.end(); ++it ) {
      Variable* var = *it;
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
  pthread_mutex_lock(&fgMutex);
  ++fgNactive;
  pthread_mutex_unlock(&fgMutex);
}

void Context::UnmarkActive()
{
  is_active = false;
  pthread_mutex_lock(&fgMutex);
  --fgNactive;
  assert( fgNactive >= 0 );
  if( fgNactive == 0 )
    pthread_cond_signal(&fgAllDone);
  pthread_mutex_unlock(&fgMutex);
}

void Context::WaitAllDone()
{
  assert( fgNctx > 0 );
  pthread_mutex_lock(&fgMutex);
  while( fgNactive > 0 )
    pthread_cond_wait(&fgAllDone, &fgMutex);
  pthread_mutex_unlock(&fgMutex);
}

bool Context::IsSyncEvent()
{
  evdata.Preload( evbuffer );
  return evdata.IsSyncEvent();
}
