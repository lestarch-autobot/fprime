// ======================================================================
// \title config/MutexSelection.hpp
// \brief selection of Posix::Mutex::PosixMutex as the Os::Mutex implementation
//
// Supplied as one of the CONFIGURATION_HEADERS of the implementation module: the build system places this file at
// config/MutexSelection.hpp when that implementation is chosen (CHOOSES_IMPLEMENTATIONS in register_fprime_config).
// Os/Mutex.hpp then defines Os::Mutex as Os::MutexCommon<Os::Selected::Mutex>.
//
// Mutex and ConditionVariable are supplied by the same implementation module: a condition variable implementation may
// operate on the handle of the selected Os::Mutex, so the two are always selected as a pair.
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
