#ifndef PPODD_UTIL
#define PPODD_UTIL

#include <algorithm>
#include <string>

//___________________________________________________________________________
template< typename Container>
class CopyObject {
public:
  CopyObject( Container& target ) : m_target(target) {}
  void operator() ( const typename Container::value_type ptr )
  {
    m_target.push_back( ptr->Clone() );
  }
private:
  Container& m_target;
};

//___________________________________________________________________________
template< typename Container >
inline void CopyContainer( const Container& from, Container& to )
{
  // Deep-copy all elements of 'from' container of pointers to 'to' Container
  // using each element's Clone() method
  to.clear();
  CopyObject<Container> copy(to);
  std::for_each( from.begin(), from.end(), copy );
}

//___________________________________________________________________________
struct DeleteObject {
  template< typename T >
  void operator() ( const T* ptr ) const { delete ptr; }
};

//___________________________________________________________________________
template< typename Container >
inline void DeleteContainer( Container& c )
{
  // Delete all elements of given container of pointers
  std::for_each( c.begin(), c.end(), DeleteObject() );
  c.clear();
}

//___________________________________________________________________________
template< typename ContainerOfContainers >
inline void DeleteContainerOfContainers( ContainerOfContainers& cc )
{
  // Delete all elements of given container of containers of pointers
  std::for_each( cc.begin(), cc.end(),
		 DeleteContainer<typename ContainerOfContainers::value_type> );
  cc.clear();
}

//___________________________________________________________________________
// Utility functions in Util.cxx

unsigned int GetThreadCount();
bool WildcardMatch( const std::string& candidate, const std::string& expr );

#endif
