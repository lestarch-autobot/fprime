// ======================================================================
// \title Os/DelegateCountingSemaphore.hpp
// \brief Define the Os::DelegateCountingSemaphore class
// ======================================================================
#ifndef OS_DELEGATECOUNTINGSEMAPHORE_HPP_
#define OS_DELEGATECOUNTINGSEMAPHORE_HPP_

#include "Os/CountingSemaphoreInterface.hpp"

namespace Os {

//! \brief Link-time delegating CountingSemaphore implementation.
//!
//! Stores an implementation-defined semaphore in a byte array and forwards all operations to a delegate
//! constructed (via placement-new) by CountingSemaphoreInterface::getDelegate(). Which getDelegate() is linked
//! selects the concrete implementation at link time. This is the default binding of the Os::CountingSemaphore
//! alias; platforms may instead alias Os::CountingSemaphore directly to a concrete implementation for
//! compile-time selection (see config/OsSelection.hpp).
class DelegateCountingSemaphore final : public CountingSemaphoreInterface {
  public:
    explicit DelegateCountingSemaphore(U32 initial_count);  //!< Constructor with initial count

    ~DelegateCountingSemaphore() final;  //!< Destructor

    //! \brief copy constructor is forbidden
    DelegateCountingSemaphore(const DelegateCountingSemaphore& other) = delete;

    //! \brief assignment operator is forbidden
    DelegateCountingSemaphore& operator=(const DelegateCountingSemaphore& other) = delete;

    Status wait() override;  //!< wait (decrement), blocking if count is zero

    Status waitTimeout(const Fw::TimeInterval& interval) override;  //!< wait with timeout

    Status tryWait() override;  //!< non-blocking attempt to decrement

    Status post() override;  //!< post (increment)

    //! \brief return the underlying semaphore handle (implementation specific)
    CountingSemaphoreHandle* getHandle() override;

  private:
    // Opaque storage for the implementation; `m_delegate` refers to the object placed in it
    alignas(FW_HANDLE_ALIGNMENT) CountingSemaphoreHandleStorage m_handle_storage;  //!< Semaphore handle storage
    CountingSemaphoreInterface& m_delegate;  //!< Delegate for the real implementation
};

}  // namespace Os

#endif  // OS_DELEGATECOUNTINGSEMAPHORE_HPP_
