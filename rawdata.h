// Raw data structures for analyzer parallelization demo

#ifndef PPODD_RAWDATA
#define PPODD_RAWDATA

#include <cstdint>

using EvDat_t = double;

static const int32_t MAXDATA = 16;
static const int32_t MAXMODULES = 8;

// For each event
struct EventHeader {
  EventHeader() = default;
  EventHeader(uint32_t len, uint32_t info) :
          event_length(len), event_info(info) {}
  uint32_t event_length;  // length of full event (bytes), including this word
  uint32_t event_info;    // info/flags for this event
} __attribute__((aligned(8)));

// For each module in the event
struct ModuleHeader {
  ModuleHeader() = default;
  ModuleHeader(uint32_t len, uint16_t num, uint16_t ndata ) :
          module_length(len), module_number(num), module_ndata(ndata) {}
  uint32_t module_length; // length of module data (bytes), including this word
  uint16_t module_number; // module (detector) number
  uint16_t module_ndata;  // number of data values for this module
} __attribute__((aligned(8)));

struct ModuleData {
  ModuleHeader header;
  EvDat_t      data[MAXDATA];
} __attribute__((aligned(128)));

struct Event {
  EventHeader header;
  ModuleData* module[MAXMODULES];
} __attribute__((aligned(128)));

#endif
