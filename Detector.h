// Simple detector base class

#ifndef PPODD_DETECTOR
#define PPODD_DETECTOR

#include <string>

class Decoder;

class Detector {
public:
  Detector( const char* name, int imod );
  virtual ~Detector() {}

  virtual void Clear();
  virtual int  Init();
  virtual int  Decode(  Decoder& evdata ) = 0;
  virtual int  Analyze();
  virtual void Print() const;

  const std::string& GetName() const { return name; }
  const std::string& GetType() const { return type; }

protected:
  std::string name;    // Name
  std::string type;    // Type (for identifying subclass)
  int imod;            // Module number (for decoding)

  virtual int  DefineVariables( bool remove = false );
};

#endif
