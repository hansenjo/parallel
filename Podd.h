// Common include for simplified analyzer demo

#ifndef PPODD_H
#define PPODD_H

#include <vector>
#include <list>
#include <memory>
#include <string>

class Variable;
class Detector;

using varlst_t = std::list<std::unique_ptr<Variable>>;
using detlst_t = std::vector<std::unique_ptr<Detector>>;

extern int debug;

struct VarDef_t {
  std::string name;
  std::string note;
  const double* loc;
};
enum { kDefine = false, kRemove = true };

// Define global variables from list of definitions in 'defs'.
// Prepend 'prefix', if any, to all variable names.
// Newly defined variables are placed in the variable list 'varlst'.
// If 'remove' is true, remove any existing variables instead.
// Returns number of variables defined.
int DefineVarsFromList( const std::vector<VarDef_t>& defs,
                        const std::string& prefix,
                        varlst_t* varlst, bool remove = false );

void PrintVarList( varlst_t& varlst );

#define ALL(c) (c).begin(), (c).end()

#endif
