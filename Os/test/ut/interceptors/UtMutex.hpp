// ======================================================================
// \title Os/test/ut/interceptors/UtMutex.hpp
// \brief TEST ONLY: Os::Mutex implementation selected in unit test builds
// ======================================================================
// Included before the guard: Os/Mutex.hpp includes this file, and may be reached from the interface header
#include <Os/Interfaces/MutexInterface.hpp>
#ifndef OS_TEST_UT_INTERCEPTORS_UTMUTEX_HPP_
#define OS_TEST_UT_INTERCEPTORS_UTMUTEX_HPP_

#ifndef OS_SELECTED_MUTEX_HEADER
#error "OS_SELECTED_MUTEX_HEADER must be defined in config/OsSelection.hpp"
#endif
#include OS_SELECTED_MUTEX_HEADER

namespace Os {
namespace Ut {
namespace Mutex {

//! \brief TEST ONLY: wraps the selected Os::Mutex implementation so that unit tests may intercept its calls
//!
//! Each call is forwarded to the installed `Os::Test::MutexTestInterceptor`, or directly to the wrapped
//! implementation (`Os::Selected::Mutex`) when no interceptor is installed.
class UtMutex : public MutexInterface {
  public:
    UtMutex();
    ~UtMutex() override;

    MutexHandle* getHandle() override;
    Status take() override;
    Status release() override;

  private:
    Os::Selected::Mutex m_real;  //!< Implementation selected for non-test builds
};

}  // namespace Mutex
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_UTMUTEX_HPP_
