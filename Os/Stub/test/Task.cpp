#include "Os/Stub/test/Task.hpp"
#include <sys/time.h>
namespace Os {
namespace Stub {
namespace Task {
namespace Test {

StaticData StaticData::data;

TestTask::TestTask() {
    StaticData::data.lastCalled = StaticData::LastFn::CONSTRUCT_FN;
}

TestTask::~TestTask() {
    this->deregister(this);
    StaticData::data.lastCalled = StaticData::LastFn::DESTRUCT_FN;
}

void TestTask::onStart() {}

Os::TaskInterface::Status TestTask::join() {
    if (not this->prepareJoin(this)) {
        return Os::TaskInterface::Status::INVALID_STATE;
    }
    StaticData::data.lastCalled = StaticData::LastFn::JOIN_FN;
    this->completeJoin(this, StaticData::data.joinStatus);
    return StaticData::data.joinStatus;
}

void TestTask::suspend(Os::TaskInterface::SuspensionType suspensionType) {
    StaticData::data.suspensionType = suspensionType;
    StaticData::data.lastCalled = StaticData::LastFn::SUSPEND_FN;
    this->completeSuspend(this, suspensionType);
}

void TestTask::resume() {
    StaticData::data.lastCalled = StaticData::LastFn::RESUME_FN;
}

Os::TaskHandle* TestTask::getHandle() {
    return nullptr;
}

Os::TaskInterface::Status TestTask::start(const Os::TaskInterface::Arguments& arguments) {
    FW_ASSERT(arguments.m_routine != nullptr);
    const Arguments wrapped_arguments = this->prepareStart(this, arguments);
    StaticData::data.lastCalled = StaticData::LastFn::START_FN;
    this->completeStart(this, StaticData::data.startStatus, wrapped_arguments);
    return StaticData::data.startStatus;
}

Os::TaskInterface::Status TestTask::_delay(const Fw::TimeInterval& interval) {
    Os::Stub::Task::Test::StaticData::data.lastCalled = Os::Stub::Task::Test::StaticData::LastFn::DELAY_FN;
    Os::Stub::Task::Test::StaticData::data.delay = interval;

    // For testing stub, the default implementation of delay for a "task" is a busy wait. This acts as a synthetic
    // albeit inefficient implementation.
    timeval start;
    timeval end;
    if (gettimeofday(&start, nullptr) == 0) {
        end.tv_usec = (start.tv_usec + interval.getUSeconds()) % 1000000;
        end.tv_sec = start.tv_sec + interval.getSeconds() + (start.tv_usec + interval.getUSeconds()) / 1000000;
        // Bounded busy wait
        for (U64 wait = 0; wait < std::numeric_limits<U64>::max(); wait++) {
            gettimeofday(&start, nullptr);
            if (((start.tv_sec >= end.tv_sec) && (start.tv_usec >= end.tv_usec)) || (start.tv_sec > end.tv_sec)) {
                break;
            }
        }
    }
    return Os::Stub::Task::Test::StaticData::data.delayStatus;
}

bool TestTask::isCooperative() {
    return true;
}
}  // namespace Test
}  // namespace Task
}  // namespace Stub
}  // namespace Os
