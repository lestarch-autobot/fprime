// ======================================================================
// \title Os/Darwin/CountingSemaphore.hpp
// \brief Darwin definitions for Os::CountingSemaphore
// ======================================================================
// Included before the guard: the interface header may itself include this file through config/OsSelection.hpp
#include <Os/Interfaces/CountingSemaphoreInterface.hpp>
#ifndef OS_DARWIN_COUNTING_SEMAPHORE_HPP
#define OS_DARWIN_COUNTING_SEMAPHORE_HPP
#include <dispatch/dispatch.h>

namespace Os {
namespace Darwin {
namespace Semaphore {

struct DarwinCountingSemaphoreHandle : public CountingSemaphoreHandle {
    dispatch_semaphore_t m_semaphore = nullptr;
    long m_initial_count = 0;
};

class DarwinCountingSemaphore : public CountingSemaphoreInterface {
  public:
    explicit DarwinCountingSemaphore(U32 initial_count);

    ~DarwinCountingSemaphore() override;

    DarwinCountingSemaphore(const DarwinCountingSemaphore& other) = delete;

    CountingSemaphoreInterface& operator=(const CountingSemaphoreInterface& other) = delete;

    Status wait() override;

    Status waitTimeout(const Fw::TimeInterval& interval) override;

    Status tryWait() override;

    Status post() override;

    CountingSemaphoreHandle* getHandle() override;

  private:
    DarwinCountingSemaphoreHandle m_handle;
};

}  // namespace Semaphore
}  // namespace Darwin
}  // namespace Os
#endif  // OS_DARWIN_COUNTING_SEMAPHORE_HPP
