// ======================================================================
// \title Os/DelegateCpu.hpp
// \brief Define the Os::DelegateCpu class
// ======================================================================
#ifndef OS_DELEGATECPU_HPP_
#define OS_DELEGATECPU_HPP_

#include "Os/CpuInterface.hpp"

namespace Os {

//! \brief link-time selected Os::Cpu
//!
//! Wraps the implementation constructed by CpuInterface::getDelegate(), which is chosen by the linker. This is
//! the default Os::Cpu. Platforms may instead alias Os::Cpu directly to an implementation for compile-time
//! selection (see config/OsSelection.hpp).
class DelegateCpu final : public CpuInterface {
  public:
    //! \brief default constructor
    DelegateCpu();

    //! \brief default virtual destructor
    ~DelegateCpu() final;

    //! \brief copy constructor is forbidden
    DelegateCpu(const CpuInterface& other) = delete;

    //! \brief copy constructor is forbidden
    DelegateCpu(const CpuInterface* other) = delete;

    //! \brief assignment operator is forbidden
    CpuInterface& operator=(const CpuInterface& other) override = delete;

    //-----------------------------------------------------------------------------
    // Delegating methods
    //-----------------------------------------------------------------------------

    //! \brief Request the count of the CPUs detected by the system
    //!
    //! This method wraps delegates to the underlying implementation.
    //!
    //! \param cpu_count: (output) filled with CPU count on system
    //! \return: OP_OK with valid CPU count, ERROR when error occurs
    //!
    Status _getCount(FwSizeType& cpu_count) override;

    //! \brief Get the CPU tick information for a given CPU
    //!
    //! CPU ticks represent a small time slice of processor time. This will retrieve the used CPU ticks and total
    //! ticks for a given CPU. This information in a running accumulation and thus a sample-to-sample
    //! differencing is needed to see the 'realtime' changing load. This shall be done by the caller. This method wraps
    //! delegates to the underlying implementation.
    //!
    //! \param ticks: (output) filled with the tick information for the given CPU
    //! \param cpu_index: index for CPU to read. Default: 0
    //! \return:  ERROR when error occurs, OK otherwise.
    //!
    Status _getTicks(Ticks& ticks, FwSizeType cpu_index) override;

    //! \brief return the underlying cpu handle (implementation specific).
    //! \return internal task handle representation
    CpuHandle* getHandle() override;

  private:
    // This section is used to store the implementation-defined cpu handle. To Os::Cpu and fprime, this type is
    // opaque and thus normal allocation cannot be done. Instead, we allow the implementor to store then handle in
    // the byte-array here and set `handle` to that address for storage.
    alignas(FW_HANDLE_ALIGNMENT) CpuHandleStorage m_handle_storage;  //!< Storage for aligned data
    CpuInterface& m_delegate;                                        //!< Delegate for the real implementation
};
}  // namespace Os
#endif  // OS_DELEGATECPU_HPP_
