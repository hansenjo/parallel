// Simple detector base class

#ifndef PPODD_DETECTOR
#define PPODD_DETECTOR

#include "Podd.h"
#include <string>
#include <vector>
#include <memory>

class Decoder;

class Detector {
public:
  Detector( std::string name, int imod );
  virtual ~Detector() = default;

  virtual void Clear();
  [[nodiscard]] virtual std::unique_ptr<Detector> Clone() const = 0;
  virtual int Init( bool shared );
  virtual int  Decode( Decoder& evdata );
  virtual int  Analyze() = 0;
  virtual void Print() const;

  void SetVarList( varlst_t& lst ) { fVars = &lst; }

  [[nodiscard]] const std::string& GetName() const { return name; }
  [[nodiscard]] const std::string& GetType() const { return type; }

protected:
  std::string name;    // Name
  std::string type;    // Type (for identifying subclass)
  int imod;            // Module number (for decoding)

  std::vector<double> data; // Raw data

  // Pointer to list of all analysis variables. The list is held in the
  // Context that also holds this detector (see Context.h). Each detector
  // adds its particular variables to this list in the call to
  // DefineVariables at Init time.
  varlst_t* fVars;

  // Define global variables holding analysis results.
  // Returns the number of variables defined.
  // If 'remove' is true, remove any existing variables.
  //
  // These "global" variables are always thread-specific, never shared,
  // since they hold results for the event associated with the thread.
  // To be implemented by derived classes.
  virtual int DefineVariables( bool remove );

  // Read database for this detector.
  // If 'shared' is true, sets parameters shared among threads.
  virtual int ReadDatabase( bool shared );
};

#endif
