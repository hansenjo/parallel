// Simple class representing an analysis result ("global variable")

#ifndef PPODD_VARIABLE
#define PPODD_VARIABLE

#include <string>

class Variable {
public:
  Variable( const char* name, const char* note, const double* loc );

  const std::string& GetName() const { return name; }
  const std::string& GetNote() const { return note; }
  double GetValue() const { return *loc; }
  void Print() const;

private:
  std::string name;
  std::string note;
  const double* loc;

};

#endif
