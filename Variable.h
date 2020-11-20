// Simple class representing an analysis result ("global variable")

#ifndef PPODD_VARIABLE
#define PPODD_VARIABLE

#include <string>

class Variable {
public:
  Variable( std::string name, std::string note, const double* loc );

  [[nodiscard]] const std::string& GetName() const { return name; }
  [[nodiscard]] const std::string& GetNote() const { return note; }
  [[nodiscard]] double GetValue() const { return *loc; }
  void Print() const;

private:
  std::string name;
  std::string note;
  const double* loc;
};

#endif
