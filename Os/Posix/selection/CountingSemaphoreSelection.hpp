// ======================================================================
// \title config/CountingSemaphoreSelection.hpp
// \brief selection of Posix::Semaphore::PosixCountingSemaphore as the Os::CountingSemaphore implementation
//
// Supplied as one of the CONFIGURATION_HEADERS of the implementation module: the build system places this file at
// config/CountingSemaphoreSelection.hpp when that implementation is chosen (CHOOSES_IMPLEMENTATIONS in
// register_fprime_config). Os/CountingSemaphore.hpp then defines Os::CountingSemaphore as
// Os::CountingSemaphoreCommon<Os::Selected::CountingSemaphore>.
// ======================================================================
#ifndef CONFIG_COUNTINGSEMAPHORESELECTION_HPP
#define CONFIG_COUNTINGSEMAPHORESELECTION_HPP

#include <Os/Posix/CountingSemaphore.hpp>

namespace Os {
namespace Selected {
using CountingSemaphore = Posix::Semaphore::PosixCountingSemaphore;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_COUNTINGSEMAPHORESELECTION_HPP
