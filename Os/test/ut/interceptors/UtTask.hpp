// ======================================================================
// \title Os/test/ut/interceptors/UtTask.hpp
// \brief TEST ONLY: Os::Task implementation selected in unit test builds
// ======================================================================
// Included before the guard: Os/Task.hpp includes this file, and may be reached from the interface header
#include <Os/Interfaces/TaskInterface.hpp>
#ifndef OS_TEST_UT_INTERCEPTORS_UTTASK_HPP_
#define OS_TEST_UT_INTERCEPTORS_UTTASK_HPP_

#ifndef OS_SELECTED_TASK_HEADER
#error "OS_SELECTED_TASK_HEADER must be defined in config/OsSelection.hpp"
#endif
#include OS_SELECTED_TASK_HEADER

namespace Os {
namespace Ut {
namespace Task {

//! \brief TEST ONLY: wraps the selected Os::Task implementation so that unit tests may intercept its calls
//!
//! Each call is forwarded to the installed `Os::Test::TaskTestInterceptor`, or directly to the wrapped
//! implementation (`Os::Selected::Task`) when no interceptor is installed.
class UtTask : public TaskInterface {
  public:
    UtTask();
    ~UtTask() override;

    void onStart() override;
    Status join() override;
    void suspend(SuspensionType suspensionType) override;
    void resume() override;
    Status _delay(const Fw::TimeInterval& interval) override;
    bool isCooperative() override;
    TaskHandle* getHandle() override;
    Status start(const Arguments& arguments) override;

  private:
    Os::Selected::Task m_real;  //!< Implementation selected for non-test builds
};

}  // namespace Task
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_UTTASK_HPP_
