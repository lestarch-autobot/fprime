// ======================================================================
// \title Os/test/ut/implementations/UtMutex.hpp
// \brief TEST ONLY: Os::Mutex implementation selected in unit test builds
// ======================================================================
#ifndef OS_TEST_UT_IMPLEMENTATIONS_UTMUTEX_HPP_
#define OS_TEST_UT_IMPLEMENTATIONS_UTMUTEX_HPP_

#include <Os/Interfaces/MutexInterface.hpp>
#include "config/MutexSelection.hpp"

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

#endif  // OS_TEST_UT_IMPLEMENTATIONS_UTMUTEX_HPP_
