// Raw data structures for analyzer parallelization demo

#ifndef PPODD_RAWDATA
#define PPODD_RAWDATA

#include <cstdint>

// For each event
struct EventHeader {
  uint32_t event_length;  // length of full event (bytes), including this word
  uint32_t event_info;    // info/flags for this event
};

static const uint16_t MAXDATA = 16;
static const uint16_t MAXMODULES = 8;

// For each module in the event
struct ModuleHeader {
  uint32_t module_length; // length of module data (bytes), including this word
  uint16_t module_number; // module (detector) number
  uint16_t module_ndata;  // number of data values for this module
};

struct ModuleData {
  ModuleHeader header;
  double       data[MAXDATA];
};

struct Event {
  EventHeader header;
  ModuleData* module[MAXMODULES];
};

#endif
