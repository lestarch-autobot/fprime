// ======================================================================
// \title config/ConditionVariableSelection.hpp
// \brief selection of Stub::Mutex::StubConditionVariable as the Os::ConditionVariable implementation
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

#include <Os/Stub/ConditionVariable.hpp>

namespace Os {
namespace Selected {
using ConditionVariable = Stub::Mutex::StubConditionVariable;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_CONDITIONVARIABLESELECTION_HPP
