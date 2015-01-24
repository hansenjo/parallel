// Simple class representing an analysis result ("global variable")

#ifndef PPODD_VARIABLE
#define PPODD_VARIABLE

#include <string>
#include <list>

class Variable {
public:
  Variable( const char* name, const char* note, const double* loc );
  virtual ~Variable() {}

  const std::string& GetName() const { return name; }
  const std::string& GetNote() const { return note; }
  double GetValue() const { return *loc; }
  void Print() const;

private:
  std::string name;
  std::string note;
  const double* loc;

};

typedef std::list<Variable*> varlst_t;
extern varlst_t gVars;

void PrintVarList( varlst_t& varlst = gVars );

#endif
