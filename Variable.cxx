// Simple variable class

#include "Podd.h"
#include "Variable.h"
#include <iostream>
#include <cassert>
#include <algorithm>

using namespace std;

Variable::Variable( string _name, string _note, const double* _loc )
  : name(std::move(_name))
  , note(std::move(_note))
  , loc(_loc)
{
  assert(loc);
}


void Variable::Print() const
{
  cout << "VAR: " << name << " \"" << note << "\" = " << GetValue() << endl;
}

// Declared in Podd.h
void PrintVarList( const shared_ptr<varlst_t>& varlst )
{
  for_each( ALL(*varlst), [](auto& var){ var->Print(); });
}
