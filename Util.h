#ifndef PPODD_UTIL
#define PPODD_UTIL

#include <algorithm>
#include <string>
#include <ctime>

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
/**
 * @fn timespec_diff(struct timespec *, struct timespec *, struct timespec *)
 * @brief Compute the diff of two timespecs, that is a - b = result.
 * @param a the minuend
 * @param b the subtrahend
 * @param result a - b
 */
static inline void timespec_diff(struct timespec *a, struct timespec *b,
                                 struct timespec *result) {
  result->tv_sec  = a->tv_sec  - b->tv_sec;
  result->tv_nsec = a->tv_nsec - b->tv_nsec;
  if (result->tv_nsec < 0) {
    --result->tv_sec;
    result->tv_nsec += 1000000000L;
  }
}

//___________________________________________________________________________
// Utility functions in Util.cxx

unsigned int GetThreadCount();
bool WildcardMatch( const std::string& candidate, const std::string& expr );
int intRand( int min, int max );

#endif
