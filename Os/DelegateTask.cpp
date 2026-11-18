// ======================================================================
// \title Os/DelegateTask.cpp
// \brief implementation of Os::DelegateTask (link-time delegating Os::Task)
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/DelegateTask.hpp>

namespace Os {

DelegateTask::DelegateTask() : m_handle_storage(), m_delegate(*TaskInterface::getDelegate(m_handle_storage)) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    // The common bookkeeping is performed on this object, not on the delegate
    this->m_delegate.m_isDelegate = true;
}

DelegateTask::~DelegateTask() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    this->deregister();
    m_delegate.~TaskInterface();
}

DelegateTask::Status DelegateTask::start(const Arguments& arguments) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    const Arguments wrapped_arguments = this->prepareStart(arguments);
    const Status status = this->m_delegate.start(wrapped_arguments);
    this->completeStart(status, wrapped_arguments);
    return status;
}

void DelegateTask::onStart() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    this->m_delegate.onStart();
}

DelegateTask::Status DelegateTask::join() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    Status status = Status::INVALID_STATE;
    if (this->prepareJoin()) {
        status = this->m_delegate.join();
        this->completeJoin(status);
    }
    return status;
}

void DelegateTask::suspend(SuspensionType suspensionType) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<TaskInterface*>(&this->m_handle_storage[0]));
    this->m_delegate.suspend(suspensionType);
    this->completeSuspend(suspensionType);
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
