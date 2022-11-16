// Another simple detector example class
//
// This detector type performs a linear fit to the data points

#ifndef PPODD_DETECTORB
#define PPODD_DETECTORB

#include "Detector.h"

class Decoder;

class DetectorTypeB : public Detector {
public:
  DetectorTypeB( const std::string& name, int imod );
  ~DetectorTypeB() override;

  void Clear() override;
  [[nodiscard]] std::unique_ptr<Detector> Clone() const override;
  int  Analyze() override;
  void Print() const override;

protected:
  // Fit results
  double slope{};
  double inter{};
  double cov11{};
  double cov22{};
  double cov12{};
  double ndof{};
  double chi2{};

  int  DefineVariables( bool remove ) override;
};

#endif
