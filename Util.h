#ifndef PPODD_UTIL
#define PPODD_UTIL

#include <algorithm>
#include <string>

//___________________________________________________________________________
template< typename Container>
class CopyObject {
public:
  explicit CopyObject( Container& target ) : m_target(target) {}
  void operator() ( const typename Container::value_type& ptr )
  {
    m_target.push_back( std::move(ptr->Clone()) );
  }
private:
  Container& m_target;
};

//___________________________________________________________________________
template< typename Container >
inline void CopyContainer( const Container& from, Container& to )
{
  // Deep-copy all elements of 'from' container, holding unique_ptr<Element>,
  // to 'to' Container, using each element's Clone() method
  to.clear();
  CopyObject<Container> copy(to);
  std::for_each( from.begin(), from.end(), copy );
}

//___________________________________________________________________________
// Utility functions in Util.cxx

unsigned int GetThreadCount();
bool WildcardMatch( const std::string& candidate, const std::string& expr );
int intRand( int min, int max );

#endif
