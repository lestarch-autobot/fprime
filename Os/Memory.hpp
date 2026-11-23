// ======================================================================
// \title Os/Memory.hpp
// \brief common function definitions for Os::Memory
// ======================================================================
#include "Os/Os.hpp"
#include "config/OsSelection.hpp"  // defines the Os::Memory alias and OS_MEMORY_HEADER; do not remove

#ifndef OS_MEMORY_HPP_
#define OS_MEMORY_HPP_

namespace Os {

//! \brief Memory variable handle parent
class MemoryHandle {};

//! \brief types and constants shared by users of Os::Memory and by its implementations
struct MemoryUserInterface {
    using Status = Os::Generic::Status;
    using Usage = Os::Generic::UsedTotal;
};

//! \brief memory implementation
template <typename T>
class MemoryCommon final : public MemoryUserInterface {
  public:
    //! \brief default constructor
    MemoryCommon();

    //! \brief default virtual destructor
    ~MemoryCommon();

    //! \brief copy constructor is forbidden
    MemoryCommon(const MemoryCommon& other) = delete;

    //! \brief copy constructor is forbidden
    MemoryCommon(const MemoryCommon* other) = delete;

    //! \brief assignment operator is forbidden
    MemoryCommon& operator=(const MemoryCommon& other) = delete;

    //-----------------------------------------------------------------------------
    // Interface methods
    //-----------------------------------------------------------------------------
  public:
    //! \brief initialize the singleton
    static void init();

    //! \brief return singleton
    static MemoryCommon& getSingleton();

    //-----------------------------------------------------------------------------
    // Delegating methods
    //-----------------------------------------------------------------------------

    //! \brief get system memory usage
    //!
    //! This method delegates to the underlying implementation.
    //!
    //! \param memory_usage: (output) data structure used to store memory usage
    //! \return:  ERROR when error occurs, OK otherwise.
    Status _getUsage(Usage& memory_usage);

    //! \brief return the underlying memory handle (implementation specific).
    //! \return internal task handle representation
    MemoryHandle* getHandle();

    //-----------------------------------------------------------------------------
    // Static interface (singleton) methods
    //-----------------------------------------------------------------------------

    //! \brief get system memory usage
    //!
    //! This method wraps delegates to the underlying implementation.
    //!
    //! \param memory_usage: (output) data structure used to store memory usage
    //! \return: ERROR when error occurs, OK otherwise.
    static Status getUsage(Usage& memory);

  private:
    // This section is used to store the implementation-defined file handle. To Os::File and fprime, this type is
    // opaque and thus normal allocation cannot be done. Instead, we allow the implementor to store then handle in
    // the byte-array here and set `handle` to that address for storage.
    T m_delegate;  //!< Delegate for the real implementation
};

}  // namespace Os
// Validate that OS_MEMORY_HEADER was defined by config/OsSelection.hpp
#ifndef OS_MEMORY_HEADER
#error "OS_MEMORY_HEADER must be defined in config/OsSelection.hpp"
#endif

#include OS_MEMORY_HEADER

#endif  // OS_CONDITION_HPP_
