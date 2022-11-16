// Simple detector example class
//
// This detector type computes N digits of pi, where N is taken from
// the raw data input file

#ifndef PPODD_DETECTORC
#define PPODD_DETECTORC

#include "Detector.h"
#include <string>
#include <vector>

class Decoder;

class DetectorTypeC : public Detector {
public:
  DetectorTypeC( const std::string& name, int imod );
  ~DetectorTypeC() override;

  void Clear() override;
  [[nodiscard]] std::unique_ptr<Detector> Clone() const override;
  int  Analyze() override;
  void Print() const override;

protected:
  std::vector<int> m_a;       // Workspace
  std::string      m_result;  // Result as string representation of a decimal number
  double           m_ndig;    // Number of digits computed (taken from raw data)
  double           m_last5;   // Last 5 digits of result (for illustration)
  double           m_scale;   // Scale factor for number of digits input value

  int  DefineVariables( bool remove ) override;
  int  ReadDatabase( bool shared ) override;
};

#endif
