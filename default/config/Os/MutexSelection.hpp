// ======================================================================
// \title config/MutexSelection.hpp
// \brief configured selection of the Os::Mutex implementation
//
// Selects, at compile time, the implementation wrapped by Os::Mutex: include the header defining the implementation and
// name it Os::Selected::Mutex. Os/Mutex.hpp defines Os::Mutex as Os::MutexCommon<Os::Selected::Mutex>, which holds the
// implementation by value and forwards to it without virtual dispatch.
//
// The implementation header must not include Os/Mutex.hpp (it derives from Os/Interfaces/MutexInterface.hpp). The alias
// only changes the C++ type; the module providing the implementation must still be in the link.
//
// Mutex and ConditionVariable: a condition variable implementation may operate on the handle of the selected Os::Mutex
// (e.g. Os/Posix/ConditionVariable.cpp casts Os::Mutex::getHandle() to PosixMutexHandle), so the two MUST be selected
// as a compatible pair (config/MutexSelection.hpp, config/ConditionVariableSelection.hpp). Pairing a Mutex with a
// ConditionVariable that expects a different handle type is undefined behavior that the compiler cannot detect.
// ======================================================================
#ifndef CONFIG_MUTEXSELECTION_HPP
#define CONFIG_MUTEXSELECTION_HPP

#include <Os/Posix/Mutex.hpp>

namespace Os {
namespace Selected {
using Mutex = Posix::Mutex::PosixMutex;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_MUTEXSELECTION_HPP
