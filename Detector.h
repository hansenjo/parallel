// Simple detector base class

#ifndef PPODD_DETECTOR
#define PPODD_DETECTOR

#include "Variable.h"
#include <string>

class Decoder;

struct VarDef_t {
  const char* name;
  const char* note;
  const double* loc;
};
enum { kDefine = false, kRemove = true };

class Detector {
public:
  Detector( const char* name );
  virtual ~Detector() {}

  virtual void Clear();
  virtual int  Init();
  virtual int  Decode(  Decoder& evdata ) = 0;
  virtual int  Analyze();
  virtual void Print() const;

  const std::string& GetName() const { return name; }
  const std::string& GetType() const { return type; }

protected:
  std::string name;
  std::string type;

  virtual int  DefineVariables( bool remove = false );

  int DefineVarsFromList( VarDef_t* defs, const char* prefix,
			  bool remove = false, varlst_t& varlst = gVars );
};

#endif
