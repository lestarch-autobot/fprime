// ======================================================================
// \title config/ConditionVariableSelection.hpp
// \brief selection of Posix::Mutex::PosixConditionVariable as the Os::ConditionVariable implementation
//
// Supplied as one of the CONFIGURATION_HEADERS of the implementation module: the build system places this file at
// config/ConditionVariableSelection.hpp when that implementation is chosen (CHOOSES_IMPLEMENTATIONS in
// register_fprime_config). Os/Condition.hpp then defines Os::ConditionVariable as
// Os::ConditionVariableCommon<Os::Selected::ConditionVariable>.
//
// Mutex and ConditionVariable are supplied by the same implementation module: a condition variable implementation may
// operate on the handle of the selected Os::Mutex, so the two are always selected as a pair.
// ======================================================================
#ifndef CONFIG_CONDITIONVARIABLESELECTION_HPP
#define CONFIG_CONDITIONVARIABLESELECTION_HPP

#include <Os/Posix/ConditionVariable.hpp>
#include <Os/Posix/Mutex.hpp>
#include <config/MutexSelection.hpp>
#include <type_traits>

namespace Os {
namespace Selected {
using ConditionVariable = Posix::Mutex::PosixConditionVariable;
}  // namespace Selected
}  // namespace Os

// PosixConditionVariable::pend reads the handle of the mutex it is given as a PosixMutexHandle
static_assert(std::is_same<Os::Selected::Mutex, Os::Posix::Mutex::PosixMutex>::value,
              "Os_ConditionVariable_Posix requires the Os_Mutex_Posix implementation of Os::Mutex");

#endif  // CONFIG_CONDITIONVARIABLESELECTION_HPP
