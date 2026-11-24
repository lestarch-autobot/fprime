// ======================================================================
// \title Os/test/ut/interceptors/TaskTestInterceptor.hpp
// \brief TEST ONLY: interface used by unit tests to intercept Os::Task calls
//
// This interface is for unit testing only. It works with Os::Ut::Task::UtTask, the Os::Task implementation selected
// in unit test builds (BUILD_UT), and has no effect in any other build. Do not use it in flight code.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_TASKTESTINTERCEPTOR_HPP_
#define OS_TEST_UT_INTERCEPTORS_TASKTESTINTERCEPTOR_HPP_

#include <Os/Interfaces/TaskInterface.hpp>
#include <Os/test/ut/interceptors/TestInterceptor.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::Task in a unit test build
//!
//! Each function mirrors the `TaskInterface` function of the same name with one extra first argument, `real`: the
//! implementation a non-test build would have used, belonging to the `Os::Task` on which the call was made.
//! Functions that are not overridden pass through to `real`; the lifetime notifications do nothing unless
//! overridden. See Os/test/ut/interceptors/TestInterceptor.hpp for installation and threading rules.
class TaskTestInterceptor : public TaskUserInterface, public TestInterceptor<TaskTestInterceptor> {
  public:
    TaskTestInterceptor() = default;
    virtual ~TaskTestInterceptor() = default;

    //! \brief notification: an Os::Task implementation was constructed
    virtual void constructed(TaskInterface& real);

    //! \brief notification: an Os::Task implementation is being destroyed
    virtual void destructed(TaskInterface& real);

    //! \brief intercept `TaskInterface::onStart`
    virtual void onStart(TaskInterface& real);

    //! \brief intercept `TaskInterface::join`
    virtual Status join(TaskInterface& real);

    //! \brief intercept `TaskInterface::suspend`
    virtual void suspend(TaskInterface& real, SuspensionType suspensionType);

    //! \brief intercept `TaskInterface::resume`
    virtual void resume(TaskInterface& real);

    //! \brief intercept `TaskInterface::_delay`
    virtual Status _delay(TaskInterface& real, const Fw::TimeInterval& interval);

    //! \brief intercept `TaskInterface::isCooperative`
    virtual bool isCooperative(TaskInterface& real);

    //! \brief intercept `TaskInterface::getHandle`
    virtual TaskHandle* getHandle(TaskInterface& real);

    //! \brief intercept `TaskInterface::start`
    virtual Status start(TaskInterface& real, const Arguments& arguments);
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_TASKTESTINTERCEPTOR_HPP_
