// Output definition module for simple analyzer

#ifndef PPODD_OUTPUT
#define PPODD_OUTPUT

class Output {
public:
  Output();

  int Define( const char* odef_file );
  int SetOutFile( const char* odat_file );

private:

};

#endif
