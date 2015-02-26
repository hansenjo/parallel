// Output definition module for simple analyzer
// Output is CSV format, optionally gzip-compressed

#ifndef PPODD_OUTPUT
#define PPODD_OUTPUT

#include "Podd.h"
#include <vector>
#include <boost/iostreams/filtering_stream.hpp>

typedef boost::iostreams::filtering_ostream ostrm_t;

class OutputElement {
public:
  OutputElement() {}

  virtual ostrm_t& write( ostrm_t& os ) const = 0;

};

class PlainVariable : public OutputElement {
public:
  PlainVariable( Variable* var );

  virtual ostrm_t& write( ostrm_t& os ) const;

private:
  Variable* fVar;
};

std::ostream& operator<<( std::ostream& os, const OutputElement& elem );

#endif
