// ======================================================================
// \title Os/Darwin/error.hpp
// \brief header for Darwin/Mach kernel return status conversion
// ======================================================================
#ifndef OS_DARWIN_ERROR_HPP
#define OS_DARWIN_ERROR_HPP
#include "Os/CountingSemaphoreInterface.hpp"

namespace Os {
namespace Darwin {

//! Convert a dispatch_semaphore_wait return value to Os::CountingSemaphoreInterface::Status.
//! \param dispatch_result: return value from dispatch_semaphore_wait (0 = success, non-zero = timeout)
//! \return: Os::CountingSemaphoreInterface::Status representation
//!
Os::CountingSemaphoreInterface::Status dispatch_result_to_semaphore_status(long dispatch_result);

}  // namespace Darwin
}  // namespace Os
#endif
