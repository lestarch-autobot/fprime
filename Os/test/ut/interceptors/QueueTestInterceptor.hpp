// ======================================================================
// \title Os/test/ut/interceptors/QueueTestInterceptor.hpp
// \brief TEST ONLY: interface used by unit tests to intercept Os::Queue calls
//
// This interface is for unit testing only. It works with Os::Ut::Queue::UtQueue, the Os::Queue implementation selected
// in unit test builds (BUILD_UT), and has no effect in any other build. Do not use it in flight code.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_QUEUETESTINTERCEPTOR_HPP_
#define OS_TEST_UT_INTERCEPTORS_QUEUETESTINTERCEPTOR_HPP_

#include <Os/Interfaces/QueueInterface.hpp>
#include <Os/test/ut/interceptors/TestInterceptor.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::Queue in a unit test build
//!
//! Each function mirrors the `QueueInterface` function of the same name with one extra first argument, `real`: the
//! implementation a non-test build would have used, belonging to the `Os::Queue` on which the call was made.
//! Functions that are not overridden pass through to `real`; the lifetime notifications do nothing unless
//! overridden. See Os/test/ut/interceptors/TestInterceptor.hpp for installation and threading rules.
class QueueTestInterceptor : public QueueUserInterface, public TestInterceptor<QueueTestInterceptor> {
  public:
    QueueTestInterceptor() = default;
    virtual ~QueueTestInterceptor() = default;

    //! \brief notification: an Os::Queue implementation was constructed
    virtual void constructed(QueueInterface& real);

    //! \brief notification: an Os::Queue implementation is being destroyed
    virtual void destructed(QueueInterface& real);

    //! \brief intercept `QueueInterface::create`
    virtual Status create(QueueInterface& real,
                          FwEnumStoreType id,
                          const Fw::ConstStringBase& name,
                          FwSizeType depth,
                          FwSizeType messageSize);

    //! \brief intercept `QueueInterface::teardown`
    virtual void teardown(QueueInterface& real);

    //! \brief intercept `QueueInterface::send`
    virtual Status send(QueueInterface& real,
                        const U8* buffer,
                        FwSizeType size,
                        FwQueuePriorityType priority,
                        BlockingType blockType);

    //! \brief intercept `QueueInterface::receive`
    virtual Status receive(QueueInterface& real,
                           U8* destination,
                           FwSizeType capacity,
                           BlockingType blockType,
                           FwSizeType& actualSize,
                           FwQueuePriorityType& priority);

    //! \brief intercept `QueueInterface::getMessagesAvailable`
    virtual FwSizeType getMessagesAvailable(const QueueInterface& real);

    //! \brief intercept `QueueInterface::getMessageHighWaterMark`
    virtual FwSizeType getMessageHighWaterMark(const QueueInterface& real);

    //! \brief intercept `QueueInterface::getHandle`
    virtual QueueHandle* getHandle(QueueInterface& real);
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_QUEUETESTINTERCEPTOR_HPP_
