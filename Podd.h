// Common include for simplified analyzer demo

#ifndef PPODD_H
#define PPODD_H

#include <vector>
#include <list>
#include <memory>
#include <string>
#include <limits>

// Typedefs for variable and detector lists
class Variable;
class Detector;
using varlst_t = std::list<std::unique_ptr<Variable>>;
using detlst_t = std::vector<std::unique_ptr<Detector>>;

// Debug level
extern int debug;

// Program configuration
struct Config {
  Config() : nev_max(std::numeric_limits<size_t>::max()), nthreads(0), mark(0) {}
  void default_names();

  std::string input_file, odef_file, output_file, db_file;
  size_t nev_max;
  unsigned int nthreads;
  unsigned int mark;
} __attribute__((aligned(128)));

extern Config cfg;

// Name, description and value of an analysis result
struct VarDef_t {
  std::string name;
  std::string note;
  const double* loc;
} __attribute__((aligned(64)));

enum { kDefine = false, kRemove = true };

// Define analysis results variables from list of definitions in 'defs'.
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
