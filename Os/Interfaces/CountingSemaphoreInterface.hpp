// ======================================================================
// \title Os/Interfaces/CountingSemaphoreInterface.hpp
// \brief pure-virtual interface implemented by Os::CountingSemaphore implementations
// ======================================================================
// Included before the guard: Os/CountingSemaphore.hpp includes the selected implementation, which includes this file
#include <Os/CountingSemaphore.hpp>

#ifndef OS_INTERFACES_COUNTINGSEMAPHOREINTERFACE_HPP_
#define OS_INTERFACES_COUNTINGSEMAPHOREINTERFACE_HPP_

namespace Os {

class CountingSemaphoreInterface : public CountingSemaphoreUserInterface {
  public:
    //! \brief default constructor
    CountingSemaphoreInterface() = default;

    //! \brief default virtual destructor
    virtual ~CountingSemaphoreInterface() = default;

    //! \brief copy constructor is forbidden
    CountingSemaphoreInterface(const CountingSemaphoreInterface& other) = delete;

    //! \brief assignment operator is forbidden
    CountingSemaphoreInterface& operator=(const CountingSemaphoreInterface& other) = delete;

    //! \brief wait (decrement) the semaphore, blocking if count is zero
    //! \return status of the operation
    virtual Status wait() = 0;

    //! \brief wait on the semaphore with a timeout
    //! \param interval maximum time to wait
    //! \return OP_OK on success, ERROR_TIMEOUT if the interval elapsed
    virtual Status waitTimeout(const Fw::TimeInterval& interval) = 0;

    //! \brief non-blocking attempt to decrement the semaphore
    //! \return OP_OK if decremented, ERROR_TIMEOUT if count was zero
    virtual Status tryWait() = 0;

    //! \brief post (increment) the semaphore, potentially waking a waiting thread
    //! \return status of the operation
    virtual Status post() = 0;

    //! \brief return the underlying semaphore handle (implementation specific)
    //! \return internal semaphore handle representation
    virtual CountingSemaphoreHandle* getHandle() = 0;
};

}  // namespace Os

#endif  // OS_INTERFACES_COUNTINGSEMAPHOREINTERFACE_HPP_
