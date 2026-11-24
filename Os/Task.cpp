// ======================================================================
// \title Os/Task.cpp
// \brief common function implementation for Os::Task
// ======================================================================
#include "Os/Task.hpp"
#include <Fw/Types/Assert.hpp>

namespace Os {

TaskUserInterface::Arguments::Arguments(const Fw::ConstStringBase& name,
                                        const Os::TaskUserInterface::taskRoutine routine,
                                        void* const routine_argument,
                                        const FwTaskPriorityType priority,
                                        const FwSizeType stackSize,
                                        const FwSizeType cpuAffinity,
                                        const FwTaskIdType identifier)
    : m_name(name),
      m_routine(routine),
      m_routine_argument(routine_argument),
      m_priority(priority),
      m_stackSize(stackSize),
      m_cpuAffinity(cpuAffinity),
      m_identifier(identifier) {
    FW_ASSERT(routine != nullptr);
}

}  // namespace Os
