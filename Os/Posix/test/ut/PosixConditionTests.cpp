// ======================================================================
// \title Os/Posix/test/ut/PosixConditionTests.cpp
// \brief tests using posix condition variable tests
// ======================================================================
#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include "Os/Condition.hpp"
#include "Os/Mutex.hpp"
#include "STest/Random/Random.hpp"

// Wait on the variable with the mutex held while another thread notifies continuously, so that the call returns
// whether it is accepted (woken by a notification) or rejected (returns at once) and can never hang the test.
static Os::ConditionVariable::Status pendWhileNotified(Os::ConditionVariable& variable, Os::Mutex& mutex) {
    std::atomic<bool> stop(false);
    mutex.lock();
    std::thread notifier([&]() {
        while (!stop) {
            variable.notifyAll();
            std::this_thread::yield();
        }
    });
    const Os::ConditionVariable::Status status = variable.pend(mutex);
    stop = true;
    mutex.unLock();
    notifier.join();
    return status;
}

// The selected Os::ConditionVariable must reject a mutex other than the one it was first used with, and keep
// accepting the original mutex afterwards.
TEST(PosixCondition, DifferentMutexRejected) {
    Os::Mutex first;
    Os::Mutex second;
    Os::ConditionVariable variable;
    ASSERT_EQ(pendWhileNotified(variable, first), Os::ConditionVariable::Status::OP_OK);
    ASSERT_EQ(pendWhileNotified(variable, second), Os::ConditionVariable::Status::ERROR_DIFFERENT_MUTEX);
    ASSERT_EQ(pendWhileNotified(variable, first), Os::ConditionVariable::Status::OP_OK);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
