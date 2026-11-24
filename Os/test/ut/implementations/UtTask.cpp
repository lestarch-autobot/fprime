// ======================================================================
// \title Os/test/ut/implementations/UtTask.cpp
// \brief TEST ONLY: UtTask and TaskTestInterceptor default implementations
// ======================================================================
#include <Os/test/ut/implementations/UtTask.hpp>
#include <Os/test/ut/interceptors/TaskTestInterceptor.hpp>

namespace Os {
namespace Test {

void TaskTestInterceptor::constructed(TaskInterface& /*real*/) {}

void TaskTestInterceptor::destructed(TaskInterface& /*real*/) {}

void TaskTestInterceptor::onStart(TaskInterface& real) {
    real.onStart();
}

TaskTestInterceptor::Status TaskTestInterceptor::join(TaskInterface& real) {
    return real.join();
}

void TaskTestInterceptor::suspend(TaskInterface& real, SuspensionType suspensionType) {
    real.suspend(suspensionType);
}

void TaskTestInterceptor::resume(TaskInterface& real) {
    real.resume();
}

TaskTestInterceptor::Status TaskTestInterceptor::_delay(TaskInterface& real, const Fw::TimeInterval& interval) {
    return real._delay(interval);
}

bool TaskTestInterceptor::isCooperative(TaskInterface& real) {
    return real.isCooperative();
}

TaskHandle* TaskTestInterceptor::getHandle(TaskInterface& real) {
    return real.getHandle();
}

TaskTestInterceptor::Status TaskTestInterceptor::start(TaskInterface& real, const Arguments& arguments) {
    return real.start(arguments);
}

}  // namespace Test

namespace Ut {
namespace Task {

using Os::Test::TaskTestInterceptor;

UtTask::UtTask() {
    TaskTestInterceptor* interceptor = TaskTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->constructed(this->m_real);
    }
}

UtTask::~UtTask() {
    TaskTestInterceptor* interceptor = TaskTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->destructed(this->m_real);
    }
}

void UtTask::onStart() {
    TaskTestInterceptor* interceptor = TaskTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->onStart(this->m_real);
    } else {
        this->m_real.onStart();
    }
}

UtTask::Status UtTask::join() {
    TaskTestInterceptor* interceptor = TaskTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->join(this->m_real) : this->m_real.join();
}

void UtTask::suspend(SuspensionType suspensionType) {
    TaskTestInterceptor* interceptor = TaskTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->suspend(this->m_real, suspensionType);
    } else {
        this->m_real.suspend(suspensionType);
    }
}

void UtTask::resume() {
    TaskTestInterceptor* interceptor = TaskTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->resume(this->m_real);
    } else {
        this->m_real.resume();
    }
}

UtTask::Status UtTask::_delay(const Fw::TimeInterval& interval) {
    TaskTestInterceptor* interceptor = TaskTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->_delay(this->m_real, interval) : this->m_real._delay(interval);
}

bool UtTask::isCooperative() {
    TaskTestInterceptor* interceptor = TaskTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->isCooperative(this->m_real) : this->m_real.isCooperative();
}

TaskHandle* UtTask::getHandle() {
    TaskTestInterceptor* interceptor = TaskTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getHandle(this->m_real) : this->m_real.getHandle();
}

UtTask::Status UtTask::start(const Arguments& arguments) {
    TaskTestInterceptor* interceptor = TaskTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->start(this->m_real, arguments) : this->m_real.start(arguments);
}

}  // namespace Task
}  // namespace Ut
}  // namespace Os
