// Output definition module for simple analyzer
// Output is CSV format, optionally gzip-compressed

#ifndef PPODD_OUTPUT
#define PPODD_OUTPUT

#include "Podd.h"
#include <boost/iostreams/filtering_stream.hpp>
#include <string>
#include <vector>
#include <memory>

using ostrm_t = boost::iostreams::filtering_ostream;

class OutputElement {
public:
  OutputElement() = default;
  virtual ~OutputElement() = default;

  [[nodiscard]] virtual const std::string& GetName() const = 0;
  [[nodiscard]] virtual char GetType() const = 0;
  virtual ostrm_t& write( ostrm_t& os, bool headerinfo = false ) const = 0;
};

class PlainVariable : public OutputElement {
public:
  explicit PlainVariable( Variable* var ) : fVar(var) {}

  [[nodiscard]] virtual const std::string& GetName() const;
  [[nodiscard]] virtual char GetType() const { return (2<<5)+sizeof(double); }
  virtual ostrm_t& write( ostrm_t& os, bool headerinfo = false ) const;

private:
  Variable* fVar;
};

class EventNumberVariable : public OutputElement {
public:
  explicit EventNumberVariable( const int& nev ) : fNev(nev) {}

  [[nodiscard]] virtual const std::string& GetName() const { return fName; }
  [[nodiscard]] virtual char GetType() const { return sizeof(int); }
  virtual ostrm_t& write( ostrm_t& os, bool headerinfo = false ) const;

private:
  static const std::string fName;
  const int& fNev;
};

using voutp_t = std::vector<std::unique_ptr<OutputElement>>;

#endif
