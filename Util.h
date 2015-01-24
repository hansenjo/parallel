#ifndef PPODD_UTIL
#define PPODD_UTIL

#include <algorithm>

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
  for_each( c.begin(), c.end(), DeleteObject() );
  c.clear();
}

//___________________________________________________________________________
// template< typename ContainerOfContainers >
// inline void DeleteContainerOfContainers( ContainerOfContainers& cc )
// {
//   // Delete all elements of given container of containers of pointers
//   for_each( cc.begin(), cc.end(),
// 	    DeleteContainer<typename ContainerOfContainers::value_type> );
//   cc.clear();
// }

#endif
