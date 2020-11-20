// Simple detector base class

#ifndef PPODD_DETECTOR
#define PPODD_DETECTOR


#include <string>
#include <vector>

class Decoder;

class Detector {
public:
  Detector( std::string name, int imod );
  virtual ~Detector() {}

  virtual void Clear();
  virtual Detector* Clone() const = 0;
  virtual int  Init();
  virtual int  Decode( Decoder& evdata );
  virtual int  Analyze() = 0;
  virtual void Print() const;

  void SetVarList( varlst_t& lst ) { fVars = &lst; }

  const std::string& GetName() const { return name; }
  const std::string& GetType() const { return type; }

protected:
  std::string name;    // Name
  std::string type;    // Type (for identifying subclass)
  int imod;            // Module number (for decoding)

  std::vector<double> data; // Raw data

  varlst_t* fVars;

  virtual int  DefineVariables( bool remove = false );
};

#endif
