// ======================================================================
// \title Os/Stub/test/ut/StubConditionTests.cpp
// \brief interface tests for Os::ConditionVariable: the wrapper calls its implementation as expected
//
// Runs on the selected Os::ConditionVariable with a recording test interceptor in place of the implementation's
// behavior.
// ======================================================================
#include <gtest/gtest.h>
#include "Os/Condition.hpp"
#include "Os/test/ut/interceptors/ConditionVariableTestInterceptor.hpp"

namespace {

//! Records the calls Os::ConditionVariable makes on its implementation; pend returns without waiting
class Recorder : public Os::Test::ConditionVariableTestInterceptor {
  public:
    enum LastFn { NONE_FN, CONSTRUCT_FN, DESTRUCT_FN, WAIT_FN, NOTIFY_FN, NOTIFY_ALL_FN, HANDLE_FN };

    void constructed(Os::ConditionVariableInterface& real) override { this->lastCalled = CONSTRUCT_FN; }
    void destructed(Os::ConditionVariableInterface& real) override { this->lastCalled = DESTRUCT_FN; }
    Status pend(Os::ConditionVariableInterface& real, Os::Mutex& mutex) override {
        this->lastCalled = WAIT_FN;
        this->passed = &mutex;
        return Status::OP_OK;
    }
    void notify(Os::ConditionVariableInterface& real) override { this->lastCalled = NOTIFY_FN; }
    void notifyAll(Os::ConditionVariableInterface& real) override { this->lastCalled = NOTIFY_ALL_FN; }
    Os::ConditionVariableHandle* getHandle(Os::ConditionVariableInterface& real) override {
        this->lastCalled = HANDLE_FN;
        this->handle = real.getHandle();
        return this->handle;
    }

    LastFn lastCalled = NONE_FN;                    //!< Last function called
    Os::Mutex* passed = nullptr;                    //!< Last mutex passed
    Os::ConditionVariableHandle* handle = nullptr;  //!< Last handle returned
};

class Interface : public ::testing::Test {
  public:
    Recorder recorder;  //!< Installed for the duration of each test
};

}  // namespace

// Construction test
TEST_F(Interface, Construction) {
    Os::ConditionVariable variable;
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_FN);
}

// Destruct test
TEST_F(Interface, Destruction) {
    delete (new Os::ConditionVariable);
    ASSERT_EQ(recorder.lastCalled, Recorder::DESTRUCT_FN);
}

// Wait test
TEST_F(Interface, WaitNotHeld) {
    Os::Mutex mutex;
    Os::ConditionVariable variable;
    auto status = variable.pend(mutex);
    ASSERT_EQ(Os::ConditionVariable::Status::OP_OK, status);
    ASSERT_EQ(recorder.lastCalled, Recorder::WAIT_FN);
    ASSERT_EQ(recorder.passed, &mutex);
}

// Wait test
TEST_F(Interface, WaitHeld) {
    Os::Mutex mutex;
    Os::ConditionVariable variable;
    mutex.lock();
    auto status = variable.pend(mutex);
    ASSERT_EQ(Os::ConditionVariable::Status::OP_OK, status);
    ASSERT_EQ(recorder.lastCalled, Recorder::WAIT_FN);
    ASSERT_EQ(recorder.passed, &mutex);
    mutex.unLock();
}

// A different mutex than the first supplied is refused by the wrapper, without calling the implementation
TEST_F(Interface, WaitDifferentMutex) {
    Os::Mutex mutex;
    Os::Mutex other;
    Os::ConditionVariable variable;
    ASSERT_EQ(Os::ConditionVariable::Status::OP_OK, variable.pend(mutex));
    recorder.lastCalled = Recorder::NONE_FN;
    ASSERT_EQ(Os::ConditionVariable::Status::ERROR_DIFFERENT_MUTEX, variable.pend(other));
    ASSERT_EQ(recorder.lastCalled, Recorder::NONE_FN);
}

// Notify test
TEST_F(Interface, Notify) {
    Os::ConditionVariable variable;
    variable.notify();
    ASSERT_EQ(recorder.lastCalled, Recorder::NOTIFY_FN);
}

// NotifyAll test
TEST_F(Interface, NotifyAll) {
    Os::ConditionVariable variable;
    variable.notifyAll();
    ASSERT_EQ(recorder.lastCalled, Recorder::NOTIFY_ALL_FN);
}

TEST_F(Interface, Handle) {
    Os::ConditionVariable variable;
    ASSERT_EQ(variable.getHandle(), recorder.handle);
    ASSERT_NE(recorder.handle, nullptr);
    ASSERT_EQ(recorder.lastCalled, Recorder::HANDLE_FN);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
