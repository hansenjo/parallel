// Prototype parallel processing analyzer

#include <iostream>
#include <pthread.h>

using namespace std;

// Thread processing
// Loop:
// - wait for data
// - feed event data to defined analysis object(s)
// - notify someone about end of processing


int main( int argc, const char** argv )
{
  // Parse command line

  // Set up analysis object

  // Copy analysis object into thread contexts 

  // Start threads

  // Loop: Read one event and hand it off to an idle thread

  return 0;
}
