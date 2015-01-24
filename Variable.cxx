// Simple variable class

#include "Variable.h"
#include <iostream>
#include <cassert>

using namespace std;

Variable::Variable( const char* _name, const char* _note, const double* _loc )
  : name(_name), note(_note), loc(_loc)
{
  assert(loc);
}


void Variable::Print() const
{
  cout << "VAR: " << name << " \"" << note << "\" = " << GetValue() << endl;
}
