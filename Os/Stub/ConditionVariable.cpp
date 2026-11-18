// ======================================================================
// \title Os/Stub/ConditionVariable.cpp
// \brief Stub implementations for Os::ConditionVariable
// ======================================================================
#include "Os/Stub/ConditionVariable.hpp"
#include "Fw/Types/Assert.hpp"

namespace Os {
namespace Stub {
namespace Mutex {

StubConditionVariable::Status StubConditionVariable::pend(Os::Mutex& mutex) {
    const Status check = ConditionVariableInterface::checkMutex(this->m_lock, mutex);
    if (check != Status::OP_OK) {
        return check;
    }
    // This stub implementation can only be used in deployments that never need to wait on any ConditionVariable.
    // Return error if anyone ever tries to wait.
    return StubConditionVariable::Status::ERROR_NOT_IMPLEMENTED;
}
void StubConditionVariable::notify() {
    // Nobody can be waiting, because pend() always fails with ERROR_NOT_IMPLEMENTED.
    // Therefore, we can notify all waiters by doing nothing.
}
void StubConditionVariable::notifyAll() {
    // Nobody can be waiting, because pend() always fails with ERROR_NOT_IMPLEMENTED.
    // Therefore, we can notify all waiters by doing nothing.
}

ConditionVariableHandle* StubConditionVariable::getHandle() {
    return &m_handle;
}

}  // namespace Mutex
}  // namespace Stub
}  // namespace Os
