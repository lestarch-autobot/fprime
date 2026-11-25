// ======================================================================
// \title Os/Interfaces/MutexInterface.hpp
// \brief pure-virtual interface implemented by Os::Mutex implementations
// ======================================================================
#ifndef OS_INTERFACES_MUTEXINTERFACE_HPP_
#define OS_INTERFACES_MUTEXINTERFACE_HPP_

#include <Os/Interfaces/MutexUserInterface.hpp>

namespace Os {

class MutexInterface : public MutexUserInterface {
  public:
    //! \brief default constructor
    MutexInterface() = default;

    //! \brief default virtual destructor
    virtual ~MutexInterface() = default;

    //! \brief copy constructor is forbidden
    MutexInterface(const MutexInterface& other) = delete;

    //! \brief copy constructor is forbidden
    MutexInterface(const MutexInterface* other) = delete;

    //! \brief assignment operator is forbidden
    MutexInterface& operator=(const MutexInterface& other) = delete;

    //! \brief return the underlying mutex handle (implementation specific)
    //! \return internal mutex handle representation
    virtual MutexHandle* getHandle() = 0;

    virtual Status take() = 0;     //!<  lock the mutex return status
    virtual Status release() = 0;  //!<  unlock the mutex return status
};

}  // namespace Os

#endif  // OS_INTERFACES_MUTEXINTERFACE_HPP_
