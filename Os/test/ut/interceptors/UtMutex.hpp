// ======================================================================
// \title Os/test/ut/interceptors/UtMutex.hpp
// \brief TEST ONLY: Os::Mutex implementation selected in unit test builds
// ======================================================================
#include <Os/Interfaces/MutexInterface.hpp>
#ifndef OS_TEST_UT_INTERCEPTORS_UTMUTEX_HPP_
#define OS_TEST_UT_INTERCEPTORS_UTMUTEX_HPP_

// Validate that OS_UT_REAL_MUTEX_HEADER was defined by config/OsSelection.hpp
#ifndef OS_UT_REAL_MUTEX_HEADER
#error "OS_UT_REAL_MUTEX_HEADER must be defined in config/OsSelection.hpp"
#endif
#include OS_UT_REAL_MUTEX_HEADER

namespace Os {
namespace Ut {
namespace Mutex {

//! \brief TEST ONLY: implementation of Os::Mutex that lets a unit test intercept mutex calls
//!
//! Holds the implementation a non-test build would have selected (`RealMutex`, set in config/OsSelection.hpp). Each
//! call goes to the installed `Os::Test::MutexTestInterceptor`, which is handed that real implementation, or straight
//! to the real implementation when no interceptor is installed.
class UtMutex : public MutexInterface {
  public:
    //! \brief constructor: reports the construction to the installed interceptor
    UtMutex();

    //! \brief destructor: reports the destruction to the installed interceptor
    ~UtMutex() override;

    MutexHandle* getHandle() override;
    Status take() override;
    Status release() override;

  private:
    RealMutex m_real;  //!< Implementation selected for non-test builds
};

}  // namespace Mutex
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_UTMUTEX_HPP_
