// Simple detector example class

#ifndef PPODD_DETECTOR

#include <string>

class Decoder;

class Detector {
public:
  Detector( const char* name );
  virtual ~Detector() {}

  void        Clear();
  virtual int Decode(  Decoder& evdata );
  virtual int Analyze( Decoder& evdata );
  void        Print() const;

private:
  std::string name;

};

#endif
