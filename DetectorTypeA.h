// Simple detector example class
//
// This detector type computes basic statistics of the raw data

#ifndef PPODD_DETECTORA
#define PPODD_DETECTORA

#include "Detector.h"

class Decoder;

class DetectorTypeA : public Detector {
public:
  DetectorTypeA( const std::string& name, int imod );
  ~DetectorTypeA() override;

  void Clear() override;
  [[nodiscard]] std::unique_ptr<Detector> Clone() const override;
  int  Decode( Decoder& evdata ) override;
  int  Analyze() override;
  void Print() const override;

protected:
  // Statistics results
  double nval{};
  double sum{};
  double min{};
  double max{};
  double mean{};
  double geom{};

  int  DefineVariables( bool remove ) override;
};

#endif
