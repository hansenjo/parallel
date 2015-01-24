// Simple variable class

#include "Variable.h"
#include <iostream>

using namespace std;

Variable::Variable( const char* _name, double*& _loc )
  : name(_name), loc(_loc)
{
}


void Variable::Print() const
{

}
