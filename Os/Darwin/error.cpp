// ======================================================================
// \title Os/Darwin/error.cpp
// \brief Darwin dispatch semaphore return status conversion implementation
// ======================================================================
#include "Os/Darwin/error.hpp"

namespace Os {
namespace Darwin {

Os::CountingSemaphoreInterface::Status dispatch_result_to_semaphore_status(long dispatch_result) {
    // dispatch_semaphore_wait returns 0 on success, non-zero on timeout
    if (dispatch_result == 0) {
        return Os::CountingSemaphoreInterface::Status::OP_OK;
    }
    return Os::CountingSemaphoreInterface::Status::ERROR_TIMEOUT;
}

}  // namespace Darwin
}  // namespace Os
