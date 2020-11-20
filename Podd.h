// Common include for simplified analyzer demo

#ifndef PPODD_H
#define PPODD_H

#include <vector>
#include <list>
#include <memory>

class Variable;
class Detector;

using varlst_t = std::list<std::unique_ptr<Variable>>;
using detlst_t = std::vector<std::unique_ptr<Detector>>;

extern int debug;

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
