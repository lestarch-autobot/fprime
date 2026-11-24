// ======================================================================
// \title Os/Stub/test/ut/StubQueueTests.cpp
// \brief interface tests for Os::Queue: the wrapper calls its implementation as expected
//
// Runs on the selected Os::Queue with a test interceptor that replaces the implementation's behavior with an
// STL-backed priority queue whose statuses may be injected. The shared queue rules run against that fake.
// ======================================================================
#include <gtest/gtest.h>
#include <algorithm>
#include <cstring>
#include <deque>
#include <limits>
#include <map>
#include <queue>
#include "Fw/Types/Assert.hpp"
#include "Fw/Types/String.hpp"
#include "Os/Os.hpp"
#include "Os/Queue.hpp"
#include "Os/test/ut/interceptors/QueueTestInterceptor.hpp"
#include "QueueRulesDefinitions.hpp"
#include "STest/Random/Random.hpp"

namespace {

//! \brief functional fake of a queue implementation with injectable statuses
//!
//! Ignores the real implementation of each Os::Queue (other than for its handle) and keeps the messages of each
//! queue in an STL priority queue. Not thread safe: the tests using it do not support blocking.
class QueueFake : public Os::Test::QueueTestInterceptor {
  public:
    enum LastFn {
        NONE_FN,
        CONSTRUCT_FN,
        DESTRUCT_FN,
        CREATE_FN,
        SEND_FN,
        RECEIVE_FN,
        MESSAGES_FN,
        HIGH_WATER_FN,
        HANDLE_FN
    };

    void constructed(Os::QueueInterface& real) override {
        this->lastCalled = CONSTRUCT_FN;
        this->m_queues[&real] = State();
    }

    void destructed(Os::QueueInterface& real) override {
        this->lastCalled = DESTRUCT_FN;
        this->m_queues.erase(&real);
    }

    Status create(Os::QueueInterface& real,
                  FwEnumStoreType id,
                  const Fw::ConstStringBase& name_in,
                  FwSizeType depth_in,
                  FwSizeType messageSize) override {
        this->lastCalled = CREATE_FN;
        this->name = name_in;
        this->depth = depth_in;
        this->size = messageSize;
        this->m_queues[&real].m_max_depth = depth_in;
        return this->createStatus;
    }

    void teardown(Os::QueueInterface& real) override {}

    Status send(Os::QueueInterface& real,
                const U8* buffer_in,
                FwSizeType size_in,
                FwQueuePriorityType priority_in,
                BlockingType blockType_in) override {
        State& state = this->m_queues[&real];
        this->lastCalled = SEND_FN;
        this->buffer = const_cast<U8*>(buffer_in);
        this->size = size_in;
        this->priority = priority_in;
        this->blockType = blockType_in;
        if (this->sendStatus != Status::OP_OK) {
            return this->sendStatus;
        } else if (size_in > sizeof Message::data) {
            return Status::SIZE_MISMATCH;
        } else if (state.m_storage.size() >= state.m_max_depth) {
            return Status::FULL;
        }
        Message message;
        (void)std::memcpy(message.data, buffer_in, static_cast<size_t>(size_in));
        message.priority = priority_in;
        message.size = size_in;
        message.order = this->m_order_counter++;
        state.m_storage.push(message);
        state.m_high_water = std::max(state.m_high_water, static_cast<FwSizeType>(state.m_storage.size()));
        return Status::OP_OK;
    }

    Status receive(Os::QueueInterface& real,
                   U8* destination,
                   FwSizeType capacity_in,
                   BlockingType blockType_in,
                   FwSizeType& actualSize,
                   FwQueuePriorityType& priority_in) override {
        State& state = this->m_queues[&real];
        this->lastCalled = RECEIVE_FN;
        this->buffer = destination;
        this->capacity = capacity_in;
        this->blockType = blockType_in;
        if (this->receiveStatus != Status::OP_OK) {
            actualSize = this->size;
            priority_in = this->priority;
            return this->receiveStatus;
        }
        if (state.m_storage.empty()) {
            return Status::EMPTY;
        }
        Message message = state.m_storage.top();
        if (message.size > capacity_in) {
            return Status::SIZE_MISMATCH;
        }
        (void)std::memcpy(destination, message.data, static_cast<size_t>(message.size));
        priority_in = message.priority;
        actualSize = message.size;
        state.m_storage.pop();
        return Status::OP_OK;
    }

    FwSizeType getMessagesAvailable(const Os::QueueInterface& real) override {
        this->lastCalled = MESSAGES_FN;
        if (this->messages != -1) {
            return static_cast<FwSizeType>(this->messages);
        }
        return this->m_queues[&real].m_storage.size();
    }

    FwSizeType getMessageHighWaterMark(const Os::QueueInterface& real) override {
        this->lastCalled = HIGH_WATER_FN;
        if (this->highWaterMark != -1) {
            return static_cast<FwSizeType>(this->highWaterMark);
        }
        return this->m_queues[&real].m_high_water;
    }

    Os::QueueHandle* getHandle(Os::QueueInterface& real) override {
        this->lastCalled = HANDLE_FN;
        this->handle = real.getHandle();
        return this->handle;
    }

    LastFn lastCalled = NONE_FN;           //!< Last function called
    Status createStatus = Status::OP_OK;   //!< Status to return from create
    Status sendStatus = Status::OP_OK;     //!< Status to return from send, when not OP_OK
    Status receiveStatus = Status::OP_OK;  //!< Status to return from receive, when not OP_OK
    FwSignedSizeType messages = -1;        //!< Message count to return, when not -1
    FwSignedSizeType highWaterMark = -1;   //!< High water mark to return, when not -1
    Os::QueueHandle* handle = nullptr;     //!< Last handle returned
    Os::QueueString name;                  //!< Last name passed
    FwSizeType depth = 0;                  //!< Last depth passed
    FwSizeType size = 0;                   //!< Last size passed, and size to return from a failing receive
    FwSizeType capacity = 0;               //!< Last capacity passed
    U8* buffer = nullptr;                  //!< Last buffer passed
    FwQueuePriorityType priority = 0;      //!< Last priority passed, and priority to return from a failing receive
    BlockingType blockType = BlockingType::NONBLOCKING;  //!< Last blocking type passed

  private:
    struct Message {
        U8 data[QUEUE_MESSAGE_SIZE_UPPER_BOUND];
        FwQueuePriorityType priority;
        FwSizeType size;
        U64 order;
    };
    struct LessMessage {
        bool operator()(const Message& a, const Message& b) const {
            if (a.priority != b.priority) {
                return PriorityCompare()(a.priority, b.priority);
            }
            FW_ASSERT(a.order != b.order);
            return a.order > b.order;
        }
    };
    struct State {
        std::priority_queue<Message, std::deque<Message>, LessMessage> m_storage;
        FwSizeType m_high_water = 0;
        FwSizeType m_max_depth = 0;
    };
    std::map<const Os::QueueInterface*, State> m_queues;  //!< Fake state of each queue, by real implementation
    U64 m_order_counter = 0;                              //!< Orders messages of equal priority
};

//! Installed for the whole process, before any task exists
QueueFake fake;

}  // namespace

void resetInjections() {
    fake.createStatus = Os::QueueInterface::Status::OP_OK;
    fake.sendStatus = Os::QueueInterface::Status::OP_OK;
    fake.receiveStatus = Os::QueueInterface::Status::OP_OK;
    fake.messages = -1;
    fake.highWaterMark = -1;
}

// Construction test
TEST(Interface, Construction) {
    Os::Queue queue;
    ASSERT_EQ(fake.lastCalled, QueueFake::CONSTRUCT_FN);
    resetInjections();
}

// Destruct test
TEST(Interface, Destruction) {
    delete (new Os::Queue);
    ASSERT_EQ(fake.lastCalled, QueueFake::DESTRUCT_FN);
    resetInjections();
}

// Create test
TEST(Interface, Create) {
    Os::Queue queue;
    Fw::String name = "My queue";
    const FwSizeType depth = STest::Random::lowerUpper(
        std::numeric_limits<FwSizeType>::min(),
        std::min(std::numeric_limits<FwSizeType>::max(), static_cast<FwSizeType>(std::numeric_limits<U32>::max())));
    const FwSizeType messageSize = STest::Random::lowerUpper(
        std::numeric_limits<FwSizeType>::min(),
        std::min(std::numeric_limits<FwSizeType>::max(), static_cast<FwSizeType>(std::numeric_limits<U32>::max())));
    fake.createStatus = Os::QueueInterface::Status::INVALID_PRIORITY;
    Os::QueueInterface::Status status = queue.create(0, name, depth, messageSize);
    ASSERT_EQ(fake.lastCalled, QueueFake::CREATE_FN);
    ASSERT_EQ(Os::QueueInterface::Status::INVALID_PRIORITY, status);
    ASSERT_STREQ(name.toChar(), fake.name.toChar());
    ASSERT_EQ(depth, fake.depth);
    ASSERT_EQ(messageSize, fake.size);
    resetInjections();
}

// Send test
TEST(Interface, SendPointer) {
    Os::Queue queue;
    Fw::String name = "My queue";
    const FwSizeType depth = STest::Random::lowerUpper(
        std::numeric_limits<FwSizeType>::min(),
        std::min(std::numeric_limits<FwSizeType>::max(), static_cast<FwSizeType>(std::numeric_limits<U32>::max())));
    const FwSizeType messageSize = QUEUE_MESSAGE_SIZE_UPPER_BOUND;
    const FwQueuePriorityType priority =
        STest::Random::lowerUpper(0, std::min(std::numeric_limits<FwQueuePriorityType>::max(),
                                              static_cast<FwQueuePriorityType>(std::numeric_limits<U32>::max())));
    U8 buffer[messageSize];
    ASSERT_EQ(Os::QueueInterface::Status::OP_OK, queue.create(0, name, depth, messageSize));
    ASSERT_STREQ(name.toChar(), queue.getName().toChar());
    fake.sendStatus = Os::QueueInterface::Status::EMPTY;
    Os::QueueInterface::Status status =
        queue.send(buffer, sizeof buffer, priority, Os::QueueInterface::BlockingType::BLOCKING);
    ASSERT_EQ(fake.lastCalled, QueueFake::SEND_FN);
    ASSERT_EQ(Os::QueueInterface::Status::EMPTY, status);
    ASSERT_EQ(fake.buffer, &buffer[0]);
    ASSERT_EQ(fake.size, messageSize);
    ASSERT_EQ(fake.priority, priority);
    ASSERT_EQ(fake.blockType, Os::QueueInterface::BlockingType::BLOCKING);
    ASSERT_EQ(depth, queue.getDepth());
    ASSERT_EQ(messageSize, queue.getMessageSize());
    resetInjections();
}

// Send test
TEST(Interface, SendBuffer) {
    Os::Queue queue;
    Fw::String name = "My queue";
    const FwSizeType depth = STest::Random::lowerUpper(
        std::numeric_limits<FwSizeType>::min(),
        std::min(std::numeric_limits<FwSizeType>::max(), static_cast<FwSizeType>(std::numeric_limits<U32>::max())));
    const FwSizeType messageSize = QUEUE_MESSAGE_SIZE_UPPER_BOUND;
    const FwQueuePriorityType priority =
        STest::Random::lowerUpper(0, std::min(std::numeric_limits<FwQueuePriorityType>::max(),
                                              static_cast<FwQueuePriorityType>(std::numeric_limits<U32>::max())));
    U8 storage[messageSize];
    Fw::ExternalSerializeBuffer buffer(storage, sizeof storage);
    Fw::String message = "hello";
    buffer.serializeFrom(message);

    ASSERT_EQ(Os::QueueInterface::Status::OP_OK, queue.create(0, name, depth, messageSize));
    fake.sendStatus = Os::QueueInterface::Status::UNKNOWN_ERROR;
    Os::QueueInterface::Status status = queue.send(buffer, priority, Os::QueueInterface::BlockingType::NONBLOCKING);
    ASSERT_EQ(fake.lastCalled, QueueFake::SEND_FN);
    ASSERT_EQ(Os::QueueInterface::Status::UNKNOWN_ERROR, status);
    ASSERT_EQ(fake.buffer, storage);
    ASSERT_EQ(fake.size, sizeof(FwSizeStoreType) + message.length());
    ASSERT_EQ(fake.priority, priority);
    ASSERT_EQ(fake.blockType, Os::QueueInterface::BlockingType::NONBLOCKING);
    ASSERT_EQ(depth, queue.getDepth());
    ASSERT_EQ(messageSize, queue.getMessageSize());

    resetInjections();
}

// Receive test
TEST(Interface, ReceivePointer) {
    Os::Queue queue;
    Fw::String name = "My queue";
    const FwSizeType depth = STest::Random::lowerUpper(
        std::numeric_limits<FwSizeType>::min(),
        std::min(std::numeric_limits<FwSizeType>::max(), static_cast<FwSizeType>(std::numeric_limits<U32>::max())));
    const FwSizeType sizeOut = QUEUE_MESSAGE_SIZE_UPPER_BOUND;
    const FwQueuePriorityType priorityOut =
        STest::Random::lowerUpper(0, std::min(std::numeric_limits<FwQueuePriorityType>::max(),
                                              static_cast<FwQueuePriorityType>(std::numeric_limits<U32>::max())));

    FwSizeType size = QUEUE_MESSAGE_SIZE_UPPER_BOUND;
    FwQueuePriorityType priority;
    U8 storage[size];

    ASSERT_EQ(Os::QueueInterface::Status::OP_OK, queue.create(0, name, depth, size));
    fake.receiveStatus = Os::QueueInterface::Status::FULL;
    fake.size = sizeOut;
    fake.priority = priorityOut;
    Os::QueueInterface::Status status =
        queue.receive(storage, sizeof storage, Os::QueueInterface::BlockingType::NONBLOCKING, size, priority);
    ASSERT_EQ(fake.lastCalled, QueueFake::RECEIVE_FN);
    ASSERT_EQ(Os::QueueInterface::Status::FULL, status);
    ASSERT_EQ(fake.buffer, &storage[0]);
    ASSERT_EQ(fake.capacity, sizeof storage);
    ASSERT_EQ(fake.priority, priority);
    ASSERT_EQ(fake.blockType, Os::QueueInterface::BlockingType::NONBLOCKING);
    ASSERT_EQ(size, sizeOut);
    ASSERT_EQ(priority, priorityOut);
    ASSERT_EQ(depth, queue.getDepth());
    ASSERT_EQ(size, queue.getMessageSize());

    resetInjections();
}

// Receive test
TEST(Interface, ReceiveBuffer) {
    Os::Queue queue;
    Fw::String name = "My queue";
    const FwSizeType depth = STest::Random::lowerUpper(
        std::numeric_limits<FwSizeType>::min(),
        std::min(std::numeric_limits<FwSizeType>::max(), static_cast<FwSizeType>(std::numeric_limits<U32>::max())));
    const FwSizeType sizeOut = QUEUE_MESSAGE_SIZE_UPPER_BOUND;
    const FwQueuePriorityType priorityOut =
        STest::Random::lowerUpper(0, std::min(std::numeric_limits<FwQueuePriorityType>::max(),
                                              static_cast<FwQueuePriorityType>(std::numeric_limits<U32>::max())));

    FwSizeType size = QUEUE_MESSAGE_SIZE_UPPER_BOUND;
    FwQueuePriorityType priority;
    U8 storage[size];
    Fw::ExternalSerializeBuffer buffer(storage, sizeof storage);

    ASSERT_EQ(Os::QueueInterface::Status::OP_OK, queue.create(0, name, depth, size));
    fake.receiveStatus = Os::QueueInterface::Status::FULL;
    fake.size = sizeOut;
    fake.priority = priorityOut;
    Os::QueueInterface::Status status = queue.receive(buffer, Os::QueueInterface::BlockingType::NONBLOCKING, priority);
    ASSERT_EQ(fake.lastCalled, QueueFake::RECEIVE_FN);
    ASSERT_EQ(Os::QueueInterface::Status::FULL, status);
    ASSERT_EQ(fake.buffer, &storage[0]);
    ASSERT_EQ(fake.capacity, sizeof storage);
    ASSERT_EQ(fake.priority, priority);
    ASSERT_EQ(fake.blockType, Os::QueueInterface::BlockingType::NONBLOCKING);
    ASSERT_EQ(size, sizeOut);
    ASSERT_EQ(priority, priorityOut);
    ASSERT_EQ(depth, queue.getDepth());
    ASSERT_EQ(size, queue.getMessageSize());

    resetInjections();
}

TEST(Interface, MessageCount) {
    Os::Queue queue;
    Fw::String name = "My queue";
    const FwSizeType depth = 1;
    const FwSizeType messageSize = 1;
    const FwSizeType messages = STest::Random::lowerUpper(
        std::numeric_limits<FwSizeType>::min(),
        std::min(std::numeric_limits<FwSizeType>::max(), static_cast<FwSizeType>(std::numeric_limits<U32>::max())));
    ASSERT_EQ(Os::QueueInterface::Status::OP_OK, queue.create(0, name, depth, messageSize));
    fake.messages = messages;
    ASSERT_EQ(queue.getMessagesAvailable(), messages);

    resetInjections();
}

TEST(Interface, MessageHighWaterMarkCount) {
    Os::Queue queue;
    Fw::String name = "My queue";
    const FwSizeType depth = 100;
    const FwSizeType messageSize = 200;
    const FwSizeType highWater = STest::Random::lowerUpper(
        std::numeric_limits<FwSizeType>::min(),
        std::min(std::numeric_limits<FwSizeType>::max(), static_cast<FwSizeType>(std::numeric_limits<U32>::max())));
    ASSERT_EQ(Os::QueueInterface::Status::OP_OK, queue.create(0, name, depth, messageSize));
    fake.highWaterMark = highWater;
    ASSERT_EQ(queue.getMessageHighWaterMark(), highWater);

    resetInjections();
}

TEST(Interface, QueueHandle) {
    Os::Queue queue;
    ASSERT_EQ(queue.getHandle(), fake.handle);
    ASSERT_EQ(fake.lastCalled, QueueFake::HANDLE_FN);
    resetInjections();
}

int main(int argc, char** argv) {
    Os::init();
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
