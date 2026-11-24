// ======================================================================
// \title config/CountingSemaphoreSelection.hpp
// \brief configured selection of the Os::CountingSemaphore implementation
//
// Selects, at compile time, the implementation wrapped by Os::CountingSemaphore: include the header defining the
// implementation and name it Os::Selected::CountingSemaphore. Os/CountingSemaphore.hpp defines Os::CountingSemaphore as
// Os::CountingSemaphoreCommon<Os::Selected::CountingSemaphore>, which holds the implementation by value and forwards to
// it without virtual dispatch.
//
// The implementation header must not include Os/CountingSemaphore.hpp (it derives from
// Os/Interfaces/CountingSemaphoreInterface.hpp). The alias only changes the C++ type; the module providing the
// implementation must still be in the link.
// ======================================================================
#ifndef CONFIG_COUNTINGSEMAPHORESELECTION_HPP
#define CONFIG_COUNTINGSEMAPHORESELECTION_HPP

#if defined(TGT_OS_TYPE_LINUX)
#include <Os/Posix/CountingSemaphore.hpp>
#elif defined(TGT_OS_TYPE_DARWIN)
#include <Os/Darwin/CountingSemaphore.hpp>
#else
#error \
    "No default Os::CountingSemaphore implementation for this platform: override config/CountingSemaphoreSelection.hpp"
#endif

namespace Os {
namespace Selected {
#if defined(TGT_OS_TYPE_LINUX)
using CountingSemaphore = Posix::Semaphore::PosixCountingSemaphore;
#elif defined(TGT_OS_TYPE_DARWIN)
using CountingSemaphore = Darwin::Semaphore::DarwinCountingSemaphore;
#endif
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_COUNTINGSEMAPHORESELECTION_HPP
