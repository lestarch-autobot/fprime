// ======================================================================
// \title Os/Interfaces/MemoryInterface.hpp
// \brief pure-virtual interface implemented by Os::Memory implementations
// ======================================================================
#ifndef OS_INTERFACES_MEMORYINTERFACE_HPP_
#define OS_INTERFACES_MEMORYINTERFACE_HPP_

#include <Os/Interfaces/MemoryUserInterface.hpp>

namespace Os {

//! \brief interface for memory implementation
class MemoryInterface : public MemoryUserInterface {
  public:
    //! Default constructor
    MemoryInterface() = default;
    //! Default destructor
    virtual ~MemoryInterface() = default;

    //! \brief copy constructor is forbidden
    MemoryInterface(const MemoryInterface& other) = delete;  // NO_CODESONAR (cpp:S3657)

    //! \brief assignment operator is forbidden
    virtual MemoryInterface& operator=(const MemoryInterface& other) = delete;

    //! \brief get system memory usage
    //!
    //! \param memory_usage: (output) data structure used to store memory usage
    //! \return: ERROR when error occurs, OK otherwise.
    virtual Status _getUsage(Usage& memory_usage) = 0;

    //! \brief return the underlying memory handle (implementation specific).
    //! \return internal task handle representation
    virtual MemoryHandle* getHandle() = 0;
};

}  // namespace Os

#endif  // OS_INTERFACES_MEMORYINTERFACE_HPP_
