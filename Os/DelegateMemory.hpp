// ======================================================================
// \title Os/DelegateMemory.hpp
// \brief link-time delegate implementation of Os::MemoryInterface
// ======================================================================
#ifndef OS_DELEGATEMEMORY_HPP_
#define OS_DELEGATEMEMORY_HPP_

#include "Os/MemoryInterface.hpp"

namespace Os {

//! \brief memory implementation delegating to the implementation selected at link time
class DelegateMemory final : public MemoryInterface {
  public:
    //! \brief default constructor
    DelegateMemory();

    //! \brief default virtual destructor
    ~DelegateMemory() final;

    //! \brief copy constructor is forbidden
    DelegateMemory(const MemoryInterface& other) = delete;

    //! \brief copy constructor is forbidden
    DelegateMemory(const MemoryInterface* other) = delete;

    //! \brief assignment operator is forbidden
    MemoryInterface& operator=(const MemoryInterface& other) override = delete;

    //-----------------------------------------------------------------------------
    // Delegating methods
    //-----------------------------------------------------------------------------

    //! \brief get system memory usage
    //!
    //! This method delegates to the underlying implementation.
    //!
    //! \param memory_usage: (output) data structure used to store memory usage
    //! \return:  ERROR when error occurs, OK otherwise.
    Status _getUsage(Usage& memory_usage) override;

    //! \brief return the underlying memory handle (implementation specific).
    //! \return internal task handle representation
    MemoryHandle* getHandle() override;

  private:
    // Storage for the implementation-defined delegate, opaque to Os::Memory and constructed in place by
    // MemoryInterface::getDelegate.
    alignas(FW_HANDLE_ALIGNMENT) MemoryHandleStorage m_handle_storage;  //!< Storage for aligned data
    MemoryInterface& m_delegate;                                        //!< Delegate for the real implementation
};
}  // namespace Os
#endif  // OS_DELEGATEMEMORY_HPP_
