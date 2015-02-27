// Common include for simplified analyzer demo

#ifndef PPODD_H
#define PPODD_H

#include <vector>
#include <list>

class Variable;
class Detector;

typedef std::list<Variable*> varlst_t;
typedef std::vector<Detector*> detlst_t;

// Legacy global lists
extern detlst_t gDets;

extern int debug;
extern int compress_output;

// Non-class specific functions
struct VarDef_t {
  const char* name;
  const char* note;
  const double* loc;
};
enum { kDefine = false, kRemove = true };

int DefineVarsFromList( VarDef_t* defs, const char* prefix,
			varlst_t* varlst, bool remove = false );

void PrintVarList( varlst_t& varlst );

#endif
