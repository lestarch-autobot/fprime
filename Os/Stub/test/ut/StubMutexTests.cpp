// ======================================================================
// \title Os/Stub/test/ut/StubMutexTests.cpp
// \brief interface tests for Os::Mutex using a test interceptor
// This ensures the delegation of function calls happens properly
// ======================================================================
#include <gtest/gtest.h>
#include "Os/Mutex.hpp"
#include "Os/test/ut/interceptors/MutexTestInterceptor.hpp"
#include "STest/Random/Random.hpp"

//! Interceptor recording the last call made on any Os::Mutex and returning injected results, never calling `real`
class RecordingInterceptor : public Os::Test::MutexTestInterceptor {
  public:
    //! Enumeration of last function called
    enum LastFn { NONE_FN, CONSTRUCT_FN, DESTRUCT_FN, TAKE_FN, RELEASE_FN, GET_HANDLE_FN };

    void constructed(Os::MutexInterface& real) override {
        this->lastCalled = CONSTRUCT_FN;
        this->lastReal = &real;
    }

    void destructed(Os::MutexInterface& real) override {
        this->lastCalled = DESTRUCT_FN;
        this->lastReal = &real;
    }

    Os::MutexHandle* getHandle(Os::MutexInterface& real) override {
        this->lastCalled = GET_HANDLE_FN;
        this->lastReal = &real;
        return this->handle;
    }

    Status take(Os::MutexInterface& real) override {
        this->lastCalled = TAKE_FN;
        this->lastReal = &real;
        return this->takeStatus;
    }

    Status release(Os::MutexInterface& real) override {
        this->lastCalled = RELEASE_FN;
        this->lastReal = &real;
        return this->releaseStatus;
    }

    LastFn lastCalled = NONE_FN;             //!< Last function called
    Os::MutexInterface* lastReal = nullptr;  //!< Real implementation passed to the last call
    Os::MutexHandle* handle = nullptr;       //!< Handle to return from getHandle
    Status takeStatus = Status::OP_OK;       //!< Status to return from take
    Status releaseStatus = Status::OP_OK;    //!< Status to return from release
};

// Interface tests: the interceptor lives for the duration of each test
class Interface : public ::testing::Test {
  public:
    RecordingInterceptor interceptor;
};

// Ensure that Os::Mutex properly constructs its implementation
TEST_F(Interface, Construction) {
    Os::Mutex mutex;
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::CONSTRUCT_FN);
    ASSERT_NE(interceptor.lastReal, nullptr);
}

// Ensure that Os::Mutex properly destroys its implementation
TEST_F(Interface, Destruction) {
    delete (new Os::Mutex);
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::DESTRUCT_FN);
}

// Ensure that Os::Mutex properly calls the implementation take()
TEST_F(Interface, Take) {
    Os::Mutex mutex;
    interceptor.takeStatus = Os::Mutex::Status::ERROR_OTHER;
    ASSERT_EQ(mutex.take(), interceptor.takeStatus);
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::TAKE_FN);
}

// Ensure that Os::Mutex properly calls the implementation release()
TEST_F(Interface, Release) {
    Os::Mutex mutex;
    interceptor.releaseStatus = Os::Mutex::Status::ERROR_OTHER;
    ASSERT_EQ(mutex.release(), interceptor.releaseStatus);
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::RELEASE_FN);
}

// Ensure that Os::Mutex properly calls the implementation take() from lock()
TEST_F(Interface, Lock) {
    Os::Mutex mutex;
    mutex.lock();
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::TAKE_FN);
}

// Ensure that Os::Mutex properly calls the implementation release() from unLock()
TEST_F(Interface, UnLock) {
    Os::Mutex mutex;
    mutex.unLock();
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::RELEASE_FN);
}

// Ensure that Os::Mutex properly calls the implementation release() from unlock()
TEST_F(Interface, UnlockAlias) {
    Os::Mutex mutex;
    mutex.unlock();
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::RELEASE_FN);
}

// Ensure that Os::Mutex properly calls the implementation getHandle()
TEST_F(Interface, GetHandle) {
    Os::Mutex mutex;
    Os::MutexHandle handle;
    interceptor.handle = &handle;
    ASSERT_EQ(mutex.getHandle(), &handle);
    ASSERT_EQ(interceptor.lastCalled, RecordingInterceptor::GET_HANDLE_FN);
}

// Ensure that calls pass through to the real implementation once the interceptor is removed
TEST(PassThrough, NoInterceptor) {
    Os::Mutex mutex;
    ASSERT_EQ(mutex.take(), Os::Mutex::Status::OP_OK);
    ASSERT_EQ(mutex.release(), Os::Mutex::Status::OP_OK);
    ASSERT_NE(mutex.getHandle(), nullptr);
}

// Ensure that functions the interceptor does not override pass through to the real implementation
TEST(PassThrough, DefaultInterceptor) {
    Os::Test::MutexTestInterceptor interceptor;
    Os::Mutex mutex;
    ASSERT_EQ(mutex.take(), Os::Mutex::Status::OP_OK);
    ASSERT_EQ(mutex.release(), Os::Mutex::Status::OP_OK);
    ASSERT_NE(mutex.getHandle(), nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
