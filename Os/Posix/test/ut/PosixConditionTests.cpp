// ======================================================================
// \title Os/Posix/test/ut/PosixConditionTests.cpp
// \brief tests using posix condition variable tests
// ======================================================================
#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include "Os/Condition.hpp"
#include "Os/Mutex.hpp"
#include "Os/Posix/ConditionVariable.hpp"
#include "STest/Random/Random.hpp"

// Wait on the variable with the mutex held while another thread notifies continuously, so that the call returns
// whether it is accepted (woken by a notification) or rejected (returns at once) and can never hang the test.
template <typename ConditionVariableType>
static Os::ConditionVariableInterface::Status pendWhileNotified(ConditionVariableType& variable, Os::Mutex& mutex) {
    std::atomic<bool> stop(false);
    mutex.lock();
    std::thread notifier([&]() {
        while (!stop) {
            variable.notifyAll();
            std::this_thread::yield();
        }
    });
    const Os::ConditionVariableInterface::Status status = variable.pend(mutex);
    stop = true;
    mutex.unLock();
    notifier.join();
    return status;
}

// A condition variable must reject a mutex other than the one it was first used with, and keep accepting the
// original mutex afterwards.
template <typename ConditionVariableType>
static void checkDifferentMutexRejected() {
    Os::Mutex first;
    Os::Mutex second;
    ConditionVariableType variable;
    ASSERT_EQ(pendWhileNotified(variable, first), Os::ConditionVariableInterface::Status::OP_OK);
    ASSERT_EQ(pendWhileNotified(variable, second), Os::ConditionVariableInterface::Status::ERROR_DIFFERENT_MUTEX);
    ASSERT_EQ(pendWhileNotified(variable, first), Os::ConditionVariableInterface::Status::OP_OK);
}

// Through the configured Os::ConditionVariable: the delegate by default, the Posix class when aliased directly
TEST(PosixCondition, DifferentMutexRejected) {
    checkDifferentMutexRejected<Os::ConditionVariable>();
}

// On the Posix class itself, so the implementation's own check is exercised whatever the configuration
TEST(PosixCondition, DifferentMutexRejectedByImplementation) {
    checkDifferentMutexRejected<Os::Posix::Mutex::PosixConditionVariable>();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
