// ======================================================================
// \title Os/Memory.hpp
// \brief common function definitions for Os::Memory
// ======================================================================
#include <Os/Interfaces/MemoryUserInterface.hpp>
#include "Fw/Types/Assert.hpp"
#include "Os/Os.hpp"
#include "config/MemorySelection.hpp"
#ifdef BUILD_UT
#include <Os/test/ut/implementations/UtMemory.hpp>
#endif

#ifndef OS_MEMORY_HPP_
#define OS_MEMORY_HPP_

namespace Os {

template <typename T>
class MemoryCommon;

#ifdef BUILD_UT
//! \brief Os::Memory in unit test builds: wraps the implementation selected in config/MemorySelection.hpp in the
//! test-only UtMemory, so that tests may intercept its calls
using Memory = MemoryCommon<Ut::Memory::UtMemory>;
#else
//! \brief Os::Memory: the common class on the implementation selected in config/MemorySelection.hpp
using Memory = MemoryCommon<Selected::Memory>;
#endif

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
namespace Os {

template <typename T>
MemoryCommon<T>::MemoryCommon() : m_delegate() {}

template <typename T>
MemoryCommon<T>::~MemoryCommon() {}

template <typename T>
void MemoryCommon<T>::init() {
    (void)MemoryCommon::getSingleton();
}

template <typename T>
MemoryCommon<T>& MemoryCommon<T>::getSingleton() {
    static MemoryCommon _singleton;
    return _singleton;
}

template <typename T>
typename MemoryCommon<T>::Status MemoryCommon<T>::_getUsage(Os::MemoryCommon<T>::Usage& memory_usage) {
    return this->m_delegate._getUsage(memory_usage);
}

template <typename T>
typename MemoryCommon<T>::Status MemoryCommon<T>::getUsage(Os::MemoryCommon<T>::Usage& memory_usage) {
    return MemoryCommon::getSingleton()._getUsage(memory_usage);
}

template <typename T>
MemoryHandle* MemoryCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}
}  // namespace Os

#endif  // OS_CONDITION_HPP_
