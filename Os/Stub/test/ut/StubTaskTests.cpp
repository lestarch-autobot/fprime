// ======================================================================
// \title Os/Stub/test/ut/StubTaskTests.cpp
// \brief interface tests for Os::Task: the wrapper calls its implementation as expected
//
// Runs on the selected Os::Task with a test interceptor that replaces the implementation's behavior with a
// cooperative fake that starts no thread and whose statuses may be injected. The shared task rules run against it.
// ======================================================================
#include <gtest/gtest.h>
#include <sys/time.h>
#include <limits>
#include "Fw/Types/String.hpp"
#include "Os/Os.hpp"
#include "Os/Task.hpp"
#include "Os/test/ut/interceptors/TaskTestInterceptor.hpp"
#include "Os/test/ut/task/CommonTests.hpp"
#include "Os/test/ut/task/RulesHeaders.hpp"

namespace {

//! \brief fake of a cooperative task implementation with injectable statuses
//!
//! Ignores the real implementation of each Os::Task: no thread is ever started.
class TaskFake : public Os::Test::TaskTestInterceptor {
  public:
    enum LastFn { NONE_FN, CONSTRUCT_FN, DESTRUCT_FN, DELAY_FN, JOIN_FN, SUSPEND_FN, RESUME_FN, START_FN };

    void constructed(Os::TaskInterface& real) override { this->lastCalled = CONSTRUCT_FN; }

    void destructed(Os::TaskInterface& real) override { this->lastCalled = DESTRUCT_FN; }

    void onStart(Os::TaskInterface& real) override {}

    Status join(Os::TaskInterface& real) override {
        this->lastCalled = JOIN_FN;
        return this->joinStatus;
    }

    void suspend(Os::TaskInterface& real, SuspensionType suspensionType_in) override {
        this->suspensionType = suspensionType_in;
        this->lastCalled = SUSPEND_FN;
    }

    void resume(Os::TaskInterface& real) override { this->lastCalled = RESUME_FN; }

    //! Busy waits for the interval
    Status _delay(Os::TaskInterface& real, const Fw::TimeInterval& interval) override {
        this->lastCalled = DELAY_FN;
        this->delay = interval;
        timeval start;
        timeval end;
        if (gettimeofday(&start, nullptr) == 0) {
            end.tv_usec = (start.tv_usec + interval.getUSeconds()) % 1000000;
            end.tv_sec = start.tv_sec + interval.getSeconds() + (start.tv_usec + interval.getUSeconds()) / 1000000;
            for (U64 wait = 0; wait < std::numeric_limits<U64>::max(); wait++) {
                gettimeofday(&start, nullptr);
                if (((start.tv_sec >= end.tv_sec) && (start.tv_usec >= end.tv_usec)) || (start.tv_sec > end.tv_sec)) {
                    break;
                }
            }
        }
        return this->delayStatus;
    }

    bool isCooperative(Os::TaskInterface& real) override { return true; }

    Os::TaskHandle* getHandle(Os::TaskInterface& real) override { return nullptr; }

    Status start(Os::TaskInterface& real, const Arguments& arguments) override {
        this->lastCalled = START_FN;
        return this->startStatus;
    }

    //! Reset the recorded calls and injected statuses
    void reset() {
        this->lastCalled = NONE_FN;
        this->suspensionType = SuspensionType::INTENTIONAL;
        this->delay = Fw::TimeInterval();
        this->delayStatus = Status::OP_OK;
        this->joinStatus = Status::OP_OK;
        this->startStatus = Status::OP_OK;
    }

    LastFn lastCalled = NONE_FN;                                  //!< Last function called
    SuspensionType suspensionType = SuspensionType::INTENTIONAL;  //!< Last suspension type passed
    Fw::TimeInterval delay;                                       //!< Last delay passed
    Status delayStatus = Status::OP_OK;                           //!< Status to return from delay
    Status joinStatus = Status::OP_OK;                            //!< Status to return from join
    Status startStatus = Status::OP_OK;                           //!< Status to return from start
};

//! Installed for the whole process: the fake starts no thread
TaskFake fake;

}  // namespace

void testMethod(void* test) {}

// Basic file tests
//! Resets the injections of the fake before and after each test
class Interface : public ::testing::Test {
  public:
    void SetUp() override { fake.reset(); }
    void TearDown() override { fake.reset(); }
};

struct QuickRegistry : Os::TaskRegistry {
    void addTask(Os::Task* task) override { this->m_task = task; }

    void removeTask(Os::Task* task) override { this->m_task = nullptr; }
    Os::Task* m_task = nullptr;
};

// Ensure that Os::Task properly calls the implementation constructor
TEST_F(Interface, Construction) {
    Os::Task task;
    ASSERT_EQ(fake.lastCalled, TaskFake::CONSTRUCT_FN);
}

// Ensure that Os::Task properly calls the implementation destructor
TEST_F(Interface, Destruction) {
    delete (new Os::Task);
    ASSERT_EQ(fake.lastCalled, TaskFake::DESTRUCT_FN);
}

// Ensure that Os::Task properly calls the implementation start
TEST_F(Interface, Start) {
    Os::Task task;
    fake.startStatus = Os::Task::Status::UNKNOWN_ERROR;
    Os::Task::Arguments arguments(Fw::String("Task"), &testMethod, nullptr);
    ASSERT_EQ(task.start(arguments), fake.startStatus);
    ASSERT_EQ(fake.lastCalled, TaskFake::START_FN);
}

// Ensure that Os::Task properly calls the implementation start with arguments
TEST_F(Interface, StartOptionalArguments) {
    Os::Task task;
    fake.startStatus = Os::Task::Status::INVALID_STACK;
    Os::Task::Arguments arguments(Fw::String("Task"), &testMethod, nullptr);
    ASSERT_EQ(task.start(arguments), fake.startStatus);
    ASSERT_EQ(fake.lastCalled, TaskFake::START_FN);
}

// Ensure that Os::Task properly calls the implementation onStart before calling start
TEST_F(Interface, OnStart) {
    Os::Task task;
    fake.startStatus = Os::Task::Status::UNKNOWN_ERROR;
    Os::Task::Arguments arguments(Fw::String("Task"), &testMethod, nullptr);
    ASSERT_EQ(task.start(arguments), fake.startStatus);
}

// Ensure that Os::Task properly calls the implementation suspend
TEST_F(Interface, Suspend) {
    Os::Task task;
    task.suspend();
    ASSERT_EQ(fake.lastCalled, TaskFake::SUSPEND_FN);
}

// Ensure that Os::Task properly calls the implementation resume
TEST_F(Interface, Resume) {
    Os::Task task;
    fake.startStatus = Os::Task::Status::UNKNOWN_ERROR;
    task.resume();
    ASSERT_EQ(fake.lastCalled, TaskFake::RESUME_FN);
}

// Ensure that Os::Task properly calls the implementation join
TEST_F(Interface, Join) {
    Os::Task task;
    fake.joinStatus = Os::Task::Status::JOIN_ERROR;
    fake.startStatus = Os::Task::Status::JOIN_ERROR;
    Os::Task::Arguments arguments(Fw::String("Task"), &testMethod, nullptr);
    ASSERT_EQ(task.start(arguments), fake.startStatus);
    task.invokeRoutine();
    ASSERT_EQ(task.join(), fake.joinStatus);
    ASSERT_EQ(fake.lastCalled, TaskFake::JOIN_FN);
}

// Ensure that Os::Task properly calls the implementation getHandle
TEST_F(Interface, GetHandle) {
    Os::Task task;
}

// Ensure that Os::Task properly calls the implementation delay
TEST_F(Interface, Delay) {
    Os::Task task;
    fake.delayStatus = Os::Task::Status::DELAY_ERROR;
    ASSERT_EQ(Os::Task::delay(Fw::TimeInterval(0, 1)), fake.delayStatus);
    ASSERT_EQ(fake.lastCalled, TaskFake::DELAY_FN);
}

// The selected Os::Task is added to the registry on a successful start and removed on destruction
TEST_F(Interface, RegistryTracksTask) {
    QuickRegistry registry;
    Os::Task::registerTaskRegistry(&registry);
    Os::Task* task = new Os::Task;
    fake.startStatus = Os::Task::Status::OP_OK;
    Os::Task::Arguments arguments(Fw::String("Task"), &testMethod, nullptr);
    ASSERT_EQ(task->start(arguments), fake.startStatus);
    ASSERT_EQ(registry.m_task, task);
    delete task;
    ASSERT_EQ(registry.m_task, nullptr);
    Os::Task::registerTaskRegistry(nullptr);
    Os::Test::Task::Tester::resetNumTasks();  // Reset task count for rules-based testing
}

int main(int argc, char** argv) {
    Os::init();
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
