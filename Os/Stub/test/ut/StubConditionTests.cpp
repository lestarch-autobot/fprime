// ======================================================================
// \title Os/Stub/test/ut/StubConditionTests.cpp
// \brief tests using stub implementation for Os::ConditionVariable interface testing
// ======================================================================
#include <gtest/gtest.h>
#include "Os/Condition.hpp"
#include "Os/Stub/test/ConditionVariable.hpp"

// Os::ConditionVariable wrapper instantiated on the test stub, independent of the selection in
// config/OsSelection.hpp. It pends on the selected Os::Mutex, which the test stub does not inspect.
using ConditionVariableUnderTest =
    Os::ConditionVariableCommon<Os::Stub::ConditionVariable::Test::TestConditionVariable>;

// Construction test
TEST(Interface, Construction) {
    ConditionVariableUnderTest variable;
    ASSERT_EQ(Os::Stub::ConditionVariable::Test::StaticData::data.lastCalled,
              Os::Stub::ConditionVariable::Test::StaticData::CONSTRUCT_FN);
}

// Destruct test
TEST(Interface, Destruction) {
    delete (new ConditionVariableUnderTest);
    ASSERT_EQ(Os::Stub::ConditionVariable::Test::StaticData::data.lastCalled,
              Os::Stub::ConditionVariable::Test::StaticData::DESTRUCT_FN);
}

// Wait test
TEST(Interface, WaitNotHeld) {
    Os::Mutex mutex;
    ConditionVariableUnderTest variable;
    auto status = variable.pend(mutex);
    ASSERT_EQ(Os::ConditionVariable::Status::OP_OK, status);
    ASSERT_EQ(Os::Stub::ConditionVariable::Test::StaticData::data.lastCalled,
              Os::Stub::ConditionVariable::Test::StaticData::WAIT_FN);
}

// Wait test
TEST(Interface, WaitHeld) {
    Os::Mutex mutex;
    ConditionVariableUnderTest variable;
    mutex.lock();
    auto status = variable.pend(mutex);
    ASSERT_EQ(Os::ConditionVariable::Status::OP_OK, status);
    ASSERT_EQ(Os::Stub::ConditionVariable::Test::StaticData::data.lastCalled,
              Os::Stub::ConditionVariable::Test::StaticData::WAIT_FN);
    mutex.unLock();
}

// A different mutex than the first supplied is refused by the wrapper, without calling the implementation
TEST(Interface, WaitDifferentMutex) {
    Os::Mutex mutex;
    Os::Mutex other;
    ConditionVariableUnderTest variable;
    ASSERT_EQ(Os::ConditionVariable::Status::OP_OK, variable.pend(mutex));
    Os::Stub::ConditionVariable::Test::StaticData::data.lastCalled =
        Os::Stub::ConditionVariable::Test::StaticData::NONE_FN;
    ASSERT_EQ(Os::ConditionVariable::Status::ERROR_DIFFERENT_MUTEX, variable.pend(other));
    ASSERT_EQ(Os::Stub::ConditionVariable::Test::StaticData::data.lastCalled,
              Os::Stub::ConditionVariable::Test::StaticData::NONE_FN);
}

// Notify test
TEST(Interface, Notify) {
    ConditionVariableUnderTest variable;
    variable.notify();
    ASSERT_EQ(Os::Stub::ConditionVariable::Test::StaticData::data.lastCalled,
              Os::Stub::ConditionVariable::Test::StaticData::NOTIFY_FN);
}

// NotifyAll test
TEST(Interface, NotifyAll) {
    ConditionVariableUnderTest variable;
    variable.notifyAll();
    ASSERT_EQ(Os::Stub::ConditionVariable::Test::StaticData::data.lastCalled,
              Os::Stub::ConditionVariable::Test::StaticData::NOTIFY_ALL_FN);
}

TEST(Interface, Handle) {
    ConditionVariableUnderTest variable;
    ASSERT_EQ(variable.getHandle(), Os::Stub::ConditionVariable::Test::StaticData::data.handle);
    ASSERT_EQ(Os::Stub::ConditionVariable::Test::StaticData::data.lastCalled,
              Os::Stub::ConditionVariable::Test::StaticData::HANDLE_FN);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
