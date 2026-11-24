// ======================================================================
// \title Os/test/ut/implementations/UtCountingSemaphore.hpp
// \brief TEST ONLY: Os::CountingSemaphore implementation selected in unit test builds
// ======================================================================
// Included before the guard: Os/CountingSemaphore.hpp includes this file, and may be reached from the interface header
#include <Os/Interfaces/CountingSemaphoreInterface.hpp>
#ifndef OS_TEST_UT_IMPLEMENTATIONS_UTCOUNTINGSEMAPHORE_HPP_
#define OS_TEST_UT_IMPLEMENTATIONS_UTCOUNTINGSEMAPHORE_HPP_

#ifndef OS_SELECTED_COUNTING_SEMAPHORE_HEADER
#error "OS_SELECTED_COUNTING_SEMAPHORE_HEADER must be defined in config/OsSelection.hpp"
#endif
#include OS_SELECTED_COUNTING_SEMAPHORE_HEADER

namespace Os {
namespace Ut {
namespace CountingSemaphore {

//! \brief TEST ONLY: wraps the selected Os::CountingSemaphore implementation so that unit tests may intercept its calls
//!
//! Each call is forwarded to the installed `Os::Test::CountingSemaphoreTestInterceptor`, or directly to the wrapped
//! implementation (`Os::Selected::CountingSemaphore`) when no interceptor is installed.
class UtCountingSemaphore : public CountingSemaphoreInterface {
  public:
    explicit UtCountingSemaphore(U32 initial_count);
    ~UtCountingSemaphore() override;

    Status wait() override;
    Status waitTimeout(const Fw::TimeInterval& interval) override;
    Status tryWait() override;
    Status post() override;
    CountingSemaphoreHandle* getHandle() override;

  private:
    Os::Selected::CountingSemaphore m_real;  //!< Implementation selected for non-test builds
};

}  // namespace CountingSemaphore
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_IMPLEMENTATIONS_UTCOUNTINGSEMAPHORE_HPP_
