// Output definition module for simple analyzer
// Output is CSV format, optionally gzip-compressed

#ifndef PPODD_OUTPUT
#define PPODD_OUTPUT

#include "Podd.h"
#include <boost/iostreams/filtering_stream.hpp>
#include <string>
#include <vector>

typedef boost::iostreams::filtering_ostream ostrm_t;

class OutputElement {
public:
  OutputElement() {}

  virtual const std::string& GetName() const = 0;
  virtual char GetType() const = 0;
  virtual ostrm_t& write( ostrm_t& os, bool headerinfo = false ) const = 0;
};

class PlainVariable : public OutputElement {
public:
  PlainVariable( Variable* var ) : fVar(var) {}

  virtual const std::string& GetName() const;
  virtual char GetType() const { return (2<<5)+sizeof(double); }
  virtual ostrm_t& write( ostrm_t& os, bool headerinfo = false ) const;

private:
  Variable* fVar;
};

class EventNumberVariable : public OutputElement {
public:
  EventNumberVariable( const int& nev ) : fNev(nev) {}

  virtual const std::string& GetName() const { return fName; }
  virtual char GetType() const { return sizeof(int); }
  virtual ostrm_t& write( ostrm_t& os, bool headerinfo = false ) const;

private:
  static const std::string fName;
  const int& fNev;
};

typedef std::vector<OutputElement*> voutp_t;

#endif
