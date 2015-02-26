// Output definition module for simple analyzer
// Output is CSV format, optionally gzip-compressed

#ifndef PPODD_OUTPUT
#define PPODD_OUTPUT

#include "Podd.h"
#include <vector>

class Output {
public:
  Output();

  int Close();
  int Init( const char* odef_file, const varlst_t& varlst );
  int Process( int iev );

  void Print() const;

private:
  std::vector<Variable*> vars;
};

#endif
