// ======================================================================
// \title config/ConditionVariableSelection.hpp
// \brief configured selection of the Os::ConditionVariable implementation
//
// Selects, at compile time, the implementation wrapped by Os::ConditionVariable: include the header defining the
// implementation and name it Os::Selected::ConditionVariable. Os/Condition.hpp defines Os::ConditionVariable as
// Os::ConditionVariableCommon<Os::Selected::ConditionVariable>, which holds the implementation by value and forwards to
// it without virtual dispatch.
//
// The implementation header must not include Os/Condition.hpp (it derives from
// Os/Interfaces/ConditionVariableInterface.hpp). The alias only changes the C++ type; the module providing the
// implementation must still be in the link.
//
// Mutex and ConditionVariable: a condition variable implementation may operate on the handle of the selected Os::Mutex
// (e.g. Os/Posix/ConditionVariable.cpp casts Os::Mutex::getHandle() to PosixMutexHandle), so the two MUST be selected
// as a compatible pair (config/MutexSelection.hpp, config/ConditionVariableSelection.hpp). Pairing a Mutex with a
// ConditionVariable that expects a different handle type is undefined behavior that the compiler cannot detect.
// ======================================================================
#ifndef CONFIG_CONDITIONVARIABLESELECTION_HPP
#define CONFIG_CONDITIONVARIABLESELECTION_HPP

#include <Os/Posix/ConditionVariable.hpp>

namespace Os {
namespace Selected {
using ConditionVariable = Posix::Mutex::PosixConditionVariable;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_CONDITIONVARIABLESELECTION_HPP
