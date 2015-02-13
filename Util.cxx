// Utility functions

#include "Util.h"
#include <cstdio>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>

using namespace std;

// Count number of available CPUs. Requires glibc >= 2.6
int GetCPUcount()
{
  cpu_set_t cs;
  CPU_ZERO(&cs);
  sched_getaffinity(0, sizeof(cs), &cs);
  return CPU_COUNT(&cs);
}