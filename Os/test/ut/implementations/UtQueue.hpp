// ======================================================================
// \title Os/test/ut/implementations/UtQueue.hpp
// \brief TEST ONLY: Os::Queue implementation selected in unit test builds
// ======================================================================
#ifndef OS_TEST_UT_IMPLEMENTATIONS_UTQUEUE_HPP_
#define OS_TEST_UT_IMPLEMENTATIONS_UTQUEUE_HPP_

#include <Os/Interfaces/QueueInterface.hpp>
#include "config/QueueSelection.hpp"

namespace Os {
namespace Ut {
namespace Queue {

//! \brief TEST ONLY: wraps the selected Os::Queue implementation so that unit tests may intercept its calls
//!
//! Each call is forwarded to the installed `Os::Test::QueueTestInterceptor`, or directly to the wrapped
//! implementation (`Os::Selected::Queue`) when no interceptor is installed.
class UtQueue : public QueueInterface {
  public:
    UtQueue();
    ~UtQueue() override;

    //! \brief assignment operator is forbidden
    QueueInterface& operator=(const QueueInterface& other) override = delete;

    Status create(FwEnumStoreType id,
                  const Fw::ConstStringBase& name,
                  FwSizeType depth,
                  FwSizeType messageSize) override;
    void teardown() override;
    Status send(const U8* buffer, FwSizeType size, FwQueuePriorityType priority, BlockingType blockType) override;
    Status receive(U8* destination,
                   FwSizeType capacity,
                   BlockingType blockType,
                   FwSizeType& actualSize,
                   FwQueuePriorityType& priority) override;
    FwSizeType getMessagesAvailable() const override;
    FwSizeType getMessageHighWaterMark() const override;
    QueueHandle* getHandle() override;

  private:
    Os::Selected::Queue m_real;  //!< Implementation selected for non-test builds
};

}  // namespace Queue
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_IMPLEMENTATIONS_UTQUEUE_HPP_
