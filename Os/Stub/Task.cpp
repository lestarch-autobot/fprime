// ======================================================================
// \title Os/Stub/Task.cpp
// \brief stub implementations for Os::Task
// ======================================================================
#include "Os/Stub/Task.hpp"
namespace Os {
namespace Stub {
namespace Task {

StubTask::~StubTask() {
    this->deregister();
}

void StubTask::onStart() {}

Os::TaskInterface::Status StubTask::join() {
    if (not this->prepareJoin()) {
        return Os::TaskInterface::Status::INVALID_STATE;
    }
    const Status status = Os::TaskInterface::Status::UNKNOWN_ERROR;
    this->completeJoin(status);
    return status;
}

void StubTask::suspend(Os::TaskInterface::SuspensionType suspensionType) {
    this->completeSuspend(suspensionType);
}

void StubTask::resume() {}

Os::TaskHandle* StubTask::getHandle() {
    return nullptr;
}

Os::TaskInterface::Status StubTask::start(const Os::TaskInterface::Arguments& arguments) {
    FW_ASSERT(arguments.m_routine != nullptr);
    const Arguments wrapped_arguments = this->prepareStart(arguments);
    const Status status = Os::TaskInterface::Status::UNKNOWN_ERROR;
    this->completeStart(status, wrapped_arguments);
    return status;
}

bool StubTask::isCooperative() {
    return true;
}

Os::Task::Status StubTask::_delay(const Fw::TimeInterval& interval) {
    FW_ASSERT(false);
    return Os::Task::Status::UNKNOWN_ERROR;
}

}  // namespace Task
}  // namespace Stub
}  // namespace Os
