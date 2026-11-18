// ======================================================================
// \title Os/Stub/CountingSemaphore.hpp
// \brief Stub definitions for Os::CountingSemaphore
// ======================================================================
#ifndef OS_STUB_COUNTING_SEMAPHORE_HPP
#define OS_STUB_COUNTING_SEMAPHORE_HPP
#include <Os/CountingSemaphoreInterface.hpp>

namespace Os {
namespace Stub {
namespace Semaphore {

struct StubCountingSemaphoreHandle : public CountingSemaphoreHandle {};

class StubCountingSemaphore final : public CountingSemaphoreInterface {
  public:
    explicit StubCountingSemaphore(U32 initial_count);

    ~StubCountingSemaphore() override = default;

    StubCountingSemaphore(const StubCountingSemaphore& other) = delete;

    StubCountingSemaphore& operator=(const StubCountingSemaphore& other) = delete;

    Status wait() override;

    Status waitTimeout(const Fw::TimeInterval& interval) override;

    Status tryWait() override;

    Status post() override;

    CountingSemaphoreHandle* getHandle() override;

  private:
    StubCountingSemaphoreHandle m_handle;
};

}  // namespace Semaphore
}  // namespace Stub
}  // namespace Os
#endif  // OS_STUB_COUNTING_SEMAPHORE_HPP
