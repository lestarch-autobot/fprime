// ======================================================================
// \title Os/Mutex.hpp
// \brief common definitions for Os::Mutex
// ======================================================================
#ifndef Os_Mutex_hpp
#define Os_Mutex_hpp

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Types/Assert.hpp>
#include <Os/Os.hpp>
#include "config/OsSelection.hpp"  // defines the Os::Mutex alias and OS_MUTEX_HEADER; do not remove

namespace Os {

struct MutexHandle {};

//! \brief types shared by Os::Mutex users and implementations
struct MutexUserInterface {
    enum Status {
        OP_OK,           //!< Operation was successful
        ERROR_BUSY,      //!< Mutex is busy
        ERROR_DEADLOCK,  //!< Deadlock condition detected
        NOT_SUPPORTED,   //!< Mutex does not support operation
        ERROR_OTHER      //!< All other errors
    };
};

//! \brief Os::Mutex wrapper around the implementation `T` selected at compile time
//!
//! Holds the implementation by value and forwards every operation to it. `Os::Mutex` is this class instantiated on
//! the implementation selected in config/OsSelection.hpp.
template <typename T>
class MutexCommon final : public MutexUserInterface {
  public:
    MutexCommon() = default;   //!<  Constructor. Mutex is unlocked when created
    ~MutexCommon() = default;  //!<  Destructor

    //! \brief copy constructor is forbidden
    MutexCommon(const MutexCommon& other) = delete;

    //! \brief assignment operator is forbidden
    MutexCommon& operator=(const MutexCommon& other) = delete;

    //! \brief return the underlying mutex handle (implementation specific)
    //! \return internal mutex handle representation
    MutexHandle* getHandle();

    Status take();     //!<  lock the mutex and get return status
    Status release();  //!<  unlock the mutex and get return status

    //! \brief lock the mutex and assert success
    void lock();

    //! \brief unlock the mutex and assert success
    void unLock();

    //! \brief alias for unLock to meet BasicLockable requirements
    void unlock();

  private:
    T m_delegate;  //!< Delegate for the real implementation
};

//! \brief locks a mutex within the current scope
//!
//! The scope lock will lock the associated mutex immediately on construction, and will unlock the mutex when the
//! scoped lock is destroyed. `Os::ScopeLock` is this class instantiated on `Os::Mutex`.
template <typename M>
class ScopeLockCommon {
  public:
    //! \brief construct the scope lock
    //!
    //! Constructs the scope lock. The mutex will be locked on construction and unlocked on destruction.
    //!
    //! \param mutex: mutex to lock/unlock within the scope
    explicit ScopeLockCommon(M& mutex) : m_mutex(mutex) { this->m_mutex.lock(); }

    //! \brief destroy the scope lock, unlocking the mutex
    ~ScopeLockCommon() { this->m_mutex.unLock(); }

    //! \brief copy constructor is forbidden
    ScopeLockCommon(const ScopeLockCommon& other) = delete;

    //! \brief copy constructor is forbidden
    ScopeLockCommon(const ScopeLockCommon* other) = delete;

    //! \brief assignment operator is forbidden
    ScopeLockCommon& operator=(const ScopeLockCommon& other) = delete;

  private:
    M& m_mutex;  //!< Stores the mutex reference
};

//! \brief scope lock of the selected Os::Mutex
using ScopeLock = ScopeLockCommon<Mutex>;

}  // namespace Os

// Validate that OS_MUTEX_HEADER was defined by config/OsSelection.hpp
#ifndef OS_MUTEX_HEADER
#error "OS_MUTEX_HEADER must be defined in config/OsSelection.hpp"
#endif

#include OS_MUTEX_HEADER

namespace Os {

template <typename T>
MutexHandle* MutexCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}

template <typename T>
typename MutexCommon<T>::Status MutexCommon<T>::take() {
    return this->m_delegate.take();
}

template <typename T>
typename MutexCommon<T>::Status MutexCommon<T>::release() {
    return this->m_delegate.release();
}

template <typename T>
void MutexCommon<T>::lock() {
    const Status status = this->take();
    FW_ASSERT(status == Status::OP_OK, static_cast<FwAssertArgType>(reinterpret_cast<PlatformPointerCastType>(this)),
              status);
}

template <typename T>
void MutexCommon<T>::unLock() {
    const Status status = this->release();
    FW_ASSERT(status == Status::OP_OK, static_cast<FwAssertArgType>(reinterpret_cast<PlatformPointerCastType>(this)),
              status);
}

template <typename T>
void MutexCommon<T>::unlock() {
    this->unLock();
}

}  // namespace Os

#endif  // Os_Mutex_hpp
