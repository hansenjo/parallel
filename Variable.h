// Simple class representing an analysis result ("global variable")

#include <string>

class Variable {
public:
  Variable( const char* _name, double*& _loc );
  virtual ~Variable() {}

  double GetValue() const { return *loc; }
  void Print() const;

private:
  std::string name;
  double* loc;

};
