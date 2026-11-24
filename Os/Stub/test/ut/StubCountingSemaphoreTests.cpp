// ======================================================================
// \title Os/Stub/test/ut/StubCountingSemaphoreTests.cpp
// \brief interface tests for Os::CountingSemaphore using a test interceptor
// This ensures the delegation of function calls happens properly
// ======================================================================
#include <gtest/gtest.h>
#include "Fw/Time/TimeInterval.hpp"
#include "Os/CountingSemaphore.hpp"
#include "Os/test/ut/interceptors/CountingSemaphoreTestInterceptor.hpp"
#include "STest/Random/Random.hpp"

//! Interceptor recording the last call made on any Os::CountingSemaphore and returning injected results, never
//! calling `real`
class RecordingInterceptor : public Os::Test::CountingSemaphoreTestInterceptor {
  public:
    //! Enumeration of last function called
    enum LastFn { NONE_FN, CONSTRUCT_FN, DESTRUCT_FN, WAIT_FN, WAIT_TIMEOUT_FN, TRY_WAIT_FN, POST_FN, GET_HANDLE_FN };

    void constructed(Os::CountingSemaphoreInterface& real, U32 initial_count) override {
        this->lastCalled = CONSTRUCT_FN;
        this->lastReal = &real;
        this->initialCount = initial_count;
    }

    void destructed(Os::CountingSemaphoreInterface& real) override {
        this->lastCalled = DESTRUCT_FN;
        this->lastReal = &real;
    }

    Status wait(Os::CountingSemaphoreInterface& real) override {
        this->lastCalled = WAIT_FN;
        this->lastReal = &real;
        return this->status;
    }

    Status waitTimeout(Os::CountingSemaphoreInterface& real, const Fw::TimeInterval& interval) override {
        this->lastCalled = WAIT_TIMEOUT_FN;
        this->lastReal = &real;
        this->lastInterval = interval;
        return this->status;
    }

    Status tryWait(Os::CountingSemaphoreInterface& real) override {
        this->lastCalled = TRY_WAIT_FN;
        this->lastReal = &real;
        return this->status;
    }

    Status post(Os::CountingSemaphoreInterface& real) override {
        this->lastCalled = POST_FN;
        this->lastReal = &real;
        return this->status;
    }

    Os::CountingSemaphoreHandle* getHandle(Os::CountingSemaphoreInterface& real) override {
        this->lastCalled = GET_HANDLE_FN;
        this->lastReal = &real;
        return this->handle;
    }

    LastFn lastCalled = NONE_FN;                         //!< Last function called
    Os::CountingSemaphoreInterface* lastReal = nullptr;  //!< Real implementation passed to the last call
    U32 initialCount = 0;                                //!< Initial count of the last construction
    Fw::TimeInterval lastInterval;                       //!< Interval of the last waitTimeout call
    Os::CountingSemaphoreHandle* handle = nullptr;       //!< Handle to return from getHandle
    Status status = Status::OP_OK;                       //!< Status to return from every intercepted call
};

// Interface tests: the interceptor lives for the duration of each test
class Interface : public ::testing::Test {
  public:
    RecordingInterceptor interceptor;
};

// Ensure that Os::CountingSemaphore properly constructs its implementation with the initial count
TEST_F(Interface, Construction) {
    Os::CountingSemaphore semaphore(3);
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::CONSTRUCT_FN);
    ASSERT_NE(interceptor.lastReal, nullptr);
    ASSERT_EQ(interceptor.initialCount, 3U);
}

// Ensure that Os::CountingSemaphore properly destroys its implementation
TEST_F(Interface, Destruction) {
    delete (new Os::CountingSemaphore(0));
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::DESTRUCT_FN);
}

// Ensure that Os::CountingSemaphore properly calls the implementation wait()
TEST_F(Interface, Wait) {
    Os::CountingSemaphore semaphore(0);
    interceptor.status = Os::CountingSemaphore::Status::ERROR_OTHER;
    ASSERT_EQ(semaphore.wait(), interceptor.status);
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::WAIT_FN);
}

// Ensure that Os::CountingSemaphore properly calls the implementation waitTimeout()
TEST_F(Interface, WaitTimeout) {
    Os::CountingSemaphore semaphore(0);
    const Fw::TimeInterval interval(2, 5);
    interceptor.status = Os::CountingSemaphore::Status::ERROR_TIMEOUT;
    ASSERT_EQ(semaphore.waitTimeout(interval), interceptor.status);
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::WAIT_TIMEOUT_FN);
    ASSERT_EQ(interceptor.lastInterval, interval);
}

// Ensure that Os::CountingSemaphore properly calls the implementation tryWait()
TEST_F(Interface, TryWait) {
    Os::CountingSemaphore semaphore(0);
    interceptor.status = Os::CountingSemaphore::Status::ERROR_OTHER;
    ASSERT_EQ(semaphore.tryWait(), interceptor.status);
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::TRY_WAIT_FN);
}

// Ensure that Os::CountingSemaphore properly calls the implementation post()
TEST_F(Interface, Post) {
    Os::CountingSemaphore semaphore(0);
    interceptor.status = Os::CountingSemaphore::Status::ERROR_OTHER;
    ASSERT_EQ(semaphore.post(), interceptor.status);
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::POST_FN);
}

// Ensure that Os::CountingSemaphore properly calls the implementation getHandle()
TEST_F(Interface, GetHandle) {
    Os::CountingSemaphore semaphore(0);
    Os::CountingSemaphoreHandle handle;
    interceptor.handle = &handle;
    ASSERT_EQ(semaphore.getHandle(), &handle);
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::GET_HANDLE_FN);
}

// Ensure that calls pass through to the real implementation once the interceptor is removed
TEST(PassThrough, NoInterceptor) {
    Os::CountingSemaphore semaphore(1);
    ASSERT_EQ(semaphore.tryWait(), Os::CountingSemaphore::Status::OP_OK);
    ASSERT_NE(semaphore.tryWait(), Os::CountingSemaphore::Status::OP_OK);
    ASSERT_EQ(semaphore.post(), Os::CountingSemaphore::Status::OP_OK);
    ASSERT_EQ(semaphore.wait(), Os::CountingSemaphore::Status::OP_OK);
    ASSERT_NE(semaphore.getHandle(), nullptr);
}

// Ensure that functions the interceptor does not override pass through to the real implementation
TEST(PassThrough, DefaultInterceptor) {
    Os::Test::CountingSemaphoreTestInterceptor interceptor;
    Os::CountingSemaphore semaphore(1);
    ASSERT_EQ(semaphore.tryWait(), Os::CountingSemaphore::Status::OP_OK);
    ASSERT_NE(semaphore.tryWait(), Os::CountingSemaphore::Status::OP_OK);
    ASSERT_EQ(semaphore.post(), Os::CountingSemaphore::Status::OP_OK);
    ASSERT_NE(semaphore.getHandle(), nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
