// ======================================================================
// \title Os/DelegateTask.cpp
// \brief implementation of Os::DelegateTask (link-time delegating Os::Task)
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/DelegateTask.hpp>
#include <Os/Task.hpp>  // completes Os::Task for the TaskInterface helpers

namespace Os {

DelegateTask::DelegateTask() : m_handle_storage(), m_delegate(*TaskInterface::getDelegate(m_handle_storage)) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
}

DelegateTask::~DelegateTask() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    this->deregister(this);
    m_delegate.~TaskInterface();
}

DelegateTask::Status DelegateTask::start(const Arguments& arguments) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    const Arguments wrapped_arguments = this->prepareStart(this, arguments);
    const Status status = this->m_delegate.start(wrapped_arguments);
    this->completeStart(this, status, wrapped_arguments);
    return status;
}

void DelegateTask::onStart() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    this->m_delegate.onStart();
}

DelegateTask::Status DelegateTask::join() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    Status status = Status::INVALID_STATE;
    if (this->prepareJoin(this)) {
        status = this->m_delegate.join();
        this->completeJoin(this, status);
    }
    return status;
}

void DelegateTask::suspend(SuspensionType suspensionType) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    this->m_delegate.suspend(suspensionType);
    this->completeSuspend(this, suspensionType);
}

void DelegateTask::resume() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    this->m_delegate.resume();
}

bool DelegateTask::isCooperative() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.isCooperative();
}

TaskHandle* DelegateTask::getHandle() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getHandle();
}

DelegateTask::Status DelegateTask::_delay(const Fw::TimeInterval& interval) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate._delay(interval);
}

}  // namespace Os
