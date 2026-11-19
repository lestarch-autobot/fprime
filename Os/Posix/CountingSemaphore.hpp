// ======================================================================
// \title Os/Posix/CountingSemaphore.hpp
// \brief Posix definitions for Os::CountingSemaphore
// ======================================================================
#ifndef OS_POSIX_COUNTING_SEMAPHORE_HPP
#define OS_POSIX_COUNTING_SEMAPHORE_HPP
#include <semaphore.h>
#include <Os/CountingSemaphoreInterface.hpp>

namespace Os {
namespace Posix {
namespace Semaphore {

struct PosixCountingSemaphoreHandle : public CountingSemaphoreHandle {
    sem_t m_semaphore{};
};

class PosixCountingSemaphore final : public CountingSemaphoreInterface {
  public:
    explicit PosixCountingSemaphore(U32 initial_count);

    ~PosixCountingSemaphore() override;

    PosixCountingSemaphore(const PosixCountingSemaphore& other) = delete;

    PosixCountingSemaphore& operator=(const PosixCountingSemaphore& other) = delete;

    Status wait() override;

    Status waitTimeout(const Fw::TimeInterval& interval) override;

    Status tryWait() override;

    Status post() override;

    CountingSemaphoreHandle* getHandle() override;

  private:
    PosixCountingSemaphoreHandle m_handle;
};

}  // namespace Semaphore
}  // namespace Posix
}  // namespace Os
#endif  // OS_POSIX_COUNTING_SEMAPHORE_HPP
