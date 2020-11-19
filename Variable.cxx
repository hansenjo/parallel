// Simple variable class

#include "Podd.h"
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

// Declared in Podd.h
void PrintVarList( varlst_t& varlst )
{
  for( auto & it : varlst ) {
    it->Print();
  }
}
