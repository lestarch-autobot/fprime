// ======================================================================
// \title Os/Posix/test/ut/PosixConditionTests.cpp
// \brief tests using posix condition variable tests
// ======================================================================
#include <gtest/gtest.h>
#include <thread>
#include "Os/Condition.hpp"
#include "Os/Mutex.hpp"
#include "Os/Posix/ConditionVariable.hpp"
#include "STest/Random/Random.hpp"

// A condition variable must reject a mutex other than the one it was first used with. The check lives in the Posix
// implementation, so it is exercised both directly and through whatever Os::ConditionVariable is configured to be.
template <typename ConditionVariableType>
static void checkDifferentMutexRejected() {
    Os::Mutex first;
    Os::Mutex second;
    ConditionVariableType variable;
    bool ready = false;

    first.lock();
    std::thread notifier([&]() {
        first.lock();
        ready = true;
        first.unLock();
        variable.notify();
    });
    while (!ready) {
        ASSERT_EQ(variable.pend(first), Os::ConditionVariableInterface::Status::OP_OK);
    }
    first.unLock();
    notifier.join();

    second.lock();
    ASSERT_EQ(variable.pend(second), Os::ConditionVariableInterface::Status::ERROR_DIFFERENT_MUTEX);
    second.unLock();

    // The original mutex remains acceptable after a rejected call
    first.lock();
    variable.notify();
    first.unLock();
}

TEST(PosixCondition, DifferentMutexRejected) {
    checkDifferentMutexRejected<Os::ConditionVariable>();
}

TEST(PosixCondition, DifferentMutexRejectedByImplementation) {
    checkDifferentMutexRejected<Os::Posix::Mutex::PosixConditionVariable>();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
