// ======================================================================
// \title Os/Stub/Task.cpp
// \brief stub implementations for Os::Task
// ======================================================================
#include "Os/Stub/Task.hpp"
namespace Os {
namespace Stub {
namespace Task {

StubTask::~StubTask() {
    this->deregister(this);
}

void StubTask::onStart() {}

Os::TaskInterface::Status StubTask::join() {
    if (not this->prepareJoin(this)) {
        return Os::TaskInterface::Status::INVALID_STATE;
    }
    const Status status = Os::TaskInterface::Status::UNKNOWN_ERROR;
    this->completeJoin(this, status);
    return status;
}

void StubTask::suspend(Os::TaskInterface::SuspensionType suspensionType) {
    this->completeSuspend(this, suspensionType);
}

void StubTask::resume() {}

Os::TaskHandle* StubTask::getHandle() {
    return nullptr;
}

Os::TaskInterface::Status StubTask::start(const Os::TaskInterface::Arguments& arguments) {
    FW_ASSERT(arguments.m_routine != nullptr);
    const Arguments wrapped_arguments = this->prepareStart(this, arguments);
    const Status status = Os::TaskInterface::Status::UNKNOWN_ERROR;
    this->completeStart(this, status, wrapped_arguments);
    return status;
}

bool StubTask::isCooperative() {
    return true;
}

Os::TaskInterface::Status StubTask::_delay(const Fw::TimeInterval& interval) {
    FW_ASSERT(false);
    return Os::TaskInterface::Status::UNKNOWN_ERROR;
}

}  // namespace Task
}  // namespace Stub
}  // namespace Os
