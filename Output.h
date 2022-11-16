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
  virtual ostrm_t& write( ostrm_t& os, bool headerinfo ) const = 0;
};

class PlainVariable : public OutputElement {
public:
  explicit PlainVariable( Variable* var ) : fVar(var) {}

  [[nodiscard]] const std::string& GetName() const override;
  [[nodiscard]] char GetType() const override { return (2<<5)+sizeof(double); }
  ostrm_t& write( ostrm_t& os, bool headerinfo ) const override;

private:
  Variable* fVar;
};

class EventNumberVariable : public OutputElement {
public:
  explicit EventNumberVariable( const int& nev ) : fNev(nev) {}

  [[nodiscard]] const std::string& GetName() const override { return fName; }
  [[nodiscard]] char GetType() const override { return sizeof(int); }
  ostrm_t& write( ostrm_t& os, bool headerinfo ) const override;

private:
  static const std::string fName;
  const int& fNev;
};

using voutp_t = std::vector<std::unique_ptr<OutputElement>>;

#endif
