// Simple detector example class

#ifndef PPODD_DETECTORA
#define PPODD_DETECTORA

#include "Detector.h"

class Decoder;

class DetectorTypeA : public Detector {
public:
  DetectorTypeA( const char* name );
  virtual ~DetectorTypeA() {}

  virtual void Clear();
  virtual int  Init();
  virtual int  Decode(  Decoder& evdata );
  virtual int  Analyze( Decoder& evdata );
  virtual void Print() const;

protected:
  double sum;
  double min;
  double max;
  double mean;
  double geom;

  virtual int  DefineVariables( bool remove = false );
};

#endif
