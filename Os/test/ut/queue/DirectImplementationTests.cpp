// ======================================================================
// \title Os/test/ut/queue/DirectImplementationTests.cpp
// \brief tests of Os::QueueInterface implementations used directly (not through Os::DelegateQueue)
//
// An implementation aliased as Os::Queue (see config/OsSelection.hpp) is not wrapped by Os::DelegateQueue. These
// tests instantiate each implementation directly and check that the common code and the safety checks supplied
// by Os::QueueInterface behave as they do through the delegate.
// ======================================================================
#include <gtest/gtest.h>
#include <type_traits>
#include "Fw/Types/Serializable.hpp"
#include "Fw/Types/String.hpp"
#include "Os/Generic/LocklessPriorityQueue.hpp"
#include "Os/Generic/PriorityMemQueue.hpp"
#include "Os/Generic/PriorityQueue.hpp"
#include "Os/Queue.hpp"
#include "Os/Stub/Queue.hpp"

namespace {

constexpr FwSizeType DEPTH = 4;
constexpr FwSizeType MESSAGE_SIZE = 16;
using Status = Os::QueueInterface::Status;
using Blocking = Os::QueueInterface::BlockingType;

//! Functional implementations: create succeeds and messages round trip
template <typename T>
class DirectQueue : public ::testing::Test {
  protected:
    void TearDown() override { this->m_queue.teardown(); }
    T m_queue;
};
using Implementations =
    ::testing::Types<Os::Generic::PriorityQueue, Os::Generic::LocklessPriorityQueue, Os::Generic::PriorityMemQueue>;
TYPED_TEST_SUITE(DirectQueue, Implementations);

TYPED_TEST(DirectQueue, UninitializedSendReceive) {
    U8 buffer[MESSAGE_SIZE] = {};
    FwSizeType actualSize = 0;
    FwQueuePriorityType priority = 0;
    EXPECT_EQ(this->m_queue.send(buffer, sizeof(buffer), 0, Blocking::NONBLOCKING), Status::UNINITIALIZED);
    EXPECT_EQ(this->m_queue.receive(buffer, sizeof(buffer), Blocking::NONBLOCKING, actualSize, priority),
              Status::UNINITIALIZED);
    EXPECT_EQ(this->m_queue.getDepth(), 0u);
    EXPECT_EQ(this->m_queue.getMessageSize(), 0u);
}

TYPED_TEST(DirectQueue, CreateRecordsStateOnce) {
    const Fw::String name("direct-queue");
    ASSERT_EQ(this->m_queue.create(0, name, DEPTH, MESSAGE_SIZE), Status::OP_OK);
    EXPECT_EQ(this->m_queue.getDepth(), DEPTH);
    EXPECT_EQ(this->m_queue.getMessageSize(), MESSAGE_SIZE);
    EXPECT_STREQ(this->m_queue.getName().toChar(), name.toChar());
    // A second create is rejected rather than asserted, and leaves the queue usable
    EXPECT_EQ(this->m_queue.create(0, name, DEPTH, MESSAGE_SIZE), Status::ALREADY_CREATED);
    const U8 message[] = {1, 2, 3};
    EXPECT_EQ(this->m_queue.send(message, sizeof(message), 0, Blocking::NONBLOCKING), Status::OP_OK);
}

TYPED_TEST(DirectQueue, SizeChecks) {
    ASSERT_EQ(this->m_queue.create(0, Fw::String("direct-queue"), DEPTH, MESSAGE_SIZE), Status::OP_OK);
    U8 buffer[MESSAGE_SIZE + 1] = {};
    FwSizeType actualSize = 0;
    FwQueuePriorityType priority = 0;
    EXPECT_EQ(this->m_queue.send(buffer, MESSAGE_SIZE + 1, 0, Blocking::NONBLOCKING), Status::SIZE_MISMATCH);
    ASSERT_EQ(this->m_queue.send(buffer, MESSAGE_SIZE, 0, Blocking::NONBLOCKING), Status::OP_OK);
    EXPECT_EQ(this->m_queue.receive(buffer, MESSAGE_SIZE - 1, Blocking::NONBLOCKING, actualSize, priority),
              Status::SIZE_MISMATCH);
    // The rejected receive left the message queued
    EXPECT_EQ(this->m_queue.getMessagesAvailable(), 1u);
}

TYPED_TEST(DirectQueue, SerializedRoundTrip) {
    ASSERT_EQ(this->m_queue.create(0, Fw::String("direct-queue"), DEPTH, MESSAGE_SIZE), Status::OP_OK);
    U8 sendStorage[MESSAGE_SIZE] = {};
    U8 receiveStorage[MESSAGE_SIZE] = {};
    Fw::ExternalSerializeBuffer sent(sendStorage, sizeof(sendStorage));
    Fw::ExternalSerializeBuffer received(receiveStorage, sizeof(receiveStorage));
    ASSERT_EQ(sent.serializeFrom(static_cast<U32>(0xC0FFEE11)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(this->m_queue.send(sent, 0, Blocking::NONBLOCKING), Status::OP_OK);
    FwQueuePriorityType priority = 1;
    ASSERT_EQ(this->m_queue.receive(received, Blocking::NONBLOCKING, priority), Status::OP_OK);
    EXPECT_EQ(priority, 0);
    EXPECT_EQ(received.getSize(), sent.getSize());
    U32 value = 0;
    ASSERT_EQ(received.deserializeTo(value), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(value, 0xC0FFEE11u);
    EXPECT_EQ(this->m_queue.receive(received, Blocking::NONBLOCKING, priority), Status::EMPTY);
}

TYPED_TEST(DirectQueue, CountedOnlyWhenConfiguredQueue) {
    const FwSizeType before = Os::Queue::getNumQueues();
    ASSERT_EQ(this->m_queue.create(0, Fw::String("direct-queue"), DEPTH, MESSAGE_SIZE), Status::OP_OK);
    // Only the configured Os::Queue type is counted; other implementations are counted by their wrapper
    const FwSizeType expected = std::is_base_of<TypeParam, Os::Queue>::value ? (before + 1) : before;
    EXPECT_EQ(Os::Queue::getNumQueues(), expected);
    Os::Queue queue;
    ASSERT_EQ(queue.create(0, Fw::String("configured-queue"), DEPTH, MESSAGE_SIZE), Status::OP_OK);
    EXPECT_EQ(Os::Queue::getNumQueues(), expected + 1);
    queue.teardown();
}

TYPED_TEST(DirectQueue, NullAndZeroArgumentsAssert) {
    U8 buffer[MESSAGE_SIZE] = {};
    FwSizeType actualSize = 0;
    FwQueuePriorityType priority = 0;
    ASSERT_DEATH_IF_SUPPORTED(this->m_queue.create(0, Fw::String("direct-queue"), 0, MESSAGE_SIZE), "");
    ASSERT_DEATH_IF_SUPPORTED(this->m_queue.create(0, Fw::String("direct-queue"), DEPTH, 0), "");
    ASSERT_DEATH_IF_SUPPORTED(this->m_queue.send(nullptr, sizeof(buffer), 0, Blocking::NONBLOCKING), "");
    ASSERT_DEATH_IF_SUPPORTED(
        this->m_queue.receive(nullptr, sizeof(buffer), Blocking::NONBLOCKING, actualSize, priority), "");
}

TEST(DirectStubQueue, ChecksAndStubStatuses) {
    Os::Stub::Queue::StubQueue queue;
    U8 buffer[MESSAGE_SIZE] = {};
    FwSizeType actualSize = 0;
    FwQueuePriorityType priority = 0;
    Fw::ExternalSerializeBuffer serialized(buffer, sizeof(buffer));
    EXPECT_EQ(queue.create(0, Fw::String("stub-queue"), DEPTH, MESSAGE_SIZE), Status::UNKNOWN_ERROR);
    EXPECT_EQ(queue.getDepth(), 0u);
    EXPECT_EQ(queue.send(buffer, sizeof(buffer), 0, Blocking::NONBLOCKING), Status::UNINITIALIZED);
    EXPECT_EQ(queue.receive(buffer, sizeof(buffer), Blocking::NONBLOCKING, actualSize, priority),
              Status::UNINITIALIZED);
    EXPECT_EQ(queue.send(serialized, 0, Blocking::NONBLOCKING), Status::UNINITIALIZED);
    EXPECT_EQ(queue.receive(serialized, Blocking::NONBLOCKING, priority), Status::UNINITIALIZED);
    ASSERT_DEATH_IF_SUPPORTED(queue.create(0, Fw::String("stub-queue"), 0, MESSAGE_SIZE), "");
    ASSERT_DEATH_IF_SUPPORTED(queue.send(nullptr, sizeof(buffer), 0, Blocking::NONBLOCKING), "");
    ASSERT_DEATH_IF_SUPPORTED(queue.receive(nullptr, sizeof(buffer), Blocking::NONBLOCKING, actualSize, priority), "");
}

}  // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
