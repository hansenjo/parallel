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
  // The base class implements reading key/value pairs
  virtual int ReadDatabase( bool shared );

  // A key/value pair read from the database
  struct DBitem {
    std::string module;
    std::string key;
    double value{};
  } __attribute__((aligned(64)));
  // Collection of key/value pairs from read from the database.
  // Derived classes may extract their parameters from these items.
  std::vector<DBitem> dbitems;

private:
  int  ParseDBkey( const std::string& line, DBitem& item );
  void ParseDBline( const std::string& line );
};

#endif
