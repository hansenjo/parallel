// Output definition module for simple analyzer

#ifndef PPODD_OUTPUT
#define PPODD_OUTPUT

#include "Podd.h"
#include <vector>

class Output {
public:
  Output();

  int Close();
  int Init( const char* odat_file, const char* odef_file,
	    const varlst_t& varlst );
  int Process();

  void Print() const;

private:
  std::vector<Variable*> vars;
};

#endif
