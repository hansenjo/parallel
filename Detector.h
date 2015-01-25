// Simple detector base class

#ifndef PPODD_DETECTOR
#define PPODD_DETECTOR

#include <string>

class Decoder;

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
};

#endif
