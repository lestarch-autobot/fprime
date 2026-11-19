// ======================================================================
// \title Os/CpuInterface.hpp
// \brief Os::CpuHandle and Os::CpuInterface definitions
// ======================================================================
#ifndef OS_CPUINTERFACE_HPP_
#define OS_CPUINTERFACE_HPP_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Os/Os.hpp>
#include "config/OsSelection.hpp"  // defines Os::Cpu alias and OS_CPU_HEADER consumed by Os/Cpu.hpp; do not remove

// Validate that the Cpu selection was made by config/OsSelection.hpp. The check is here rather than in Os/Cpu.hpp
// because CpuInterface names the Os::Cpu alias below.
#ifndef OS_CPU_HEADER
#error "OS_CPU_HEADER and the Os::Cpu alias must be defined in config/OsSelection.hpp"
#endif

namespace Os {

//! \brief Cpu variable handle parent
class CpuHandle {};

//! \brief interface for cpu implementation
class CpuInterface {
  public:
    using Status = Os::Generic::Status;
    using Ticks = Os::Generic::UsedTotal;

    //! Default constructor
    CpuInterface() = default;
    //! Default destructor
    virtual ~CpuInterface() = default;

    //! \brief copy constructor is forbidden
    CpuInterface(const CpuInterface& other) = delete;

    //! \brief assignment operator is forbidden
    virtual CpuInterface& operator=(const CpuInterface& other) = delete;  //  NO_CODESONAR (cpp:S3657)

    //! \brief Request the count of the CPUs detected by the system
    //!
    //! \param cpu_count: (output) filled with CPU count on system
    //! \return: OP_OK with valid CPU count, ERROR when error occurs
    //!
    virtual Status _getCount(FwSizeType& cpu_count) = 0;

    //! \brief Get the CPU tick information for a given CPU
    //!
    //! CPU ticks represent a small time slice of processor time. This will retrieve the used CPU ticks and total
    //! ticks for a given CPU. This information in a running accumulation and thus a sample-to-sample
    //! differencing is needed to see the 'realtime' changing load. This shall be done by the caller.
    //!
    //! \param ticks: (output) filled with the tick information for the given CPU
    //! \param cpu_index: index for CPU to read. Default: 0
    //! \return:  ERROR when error occurs, OK otherwise.
    //!
    virtual Status _getTicks(Ticks& ticks, FwSizeType cpu_index) = 0;

    //! \brief return the underlying cpu handle (implementation specific).
    //! \return internal task handle representation
    virtual CpuHandle* getHandle() = 0;

    //! \brief provide a pointer to a Cpu delegate object
    static CpuInterface* getDelegate(CpuHandleStorage& aligned_new_memory);

    // ------------------------------------------------------------------
    // Common functions shared by every Os::Cpu selection
    //
    // Implemented once in terms of the configured Os::Cpu alias and the
    // functions above, so that link-time delegates and compile-time
    // selected implementations behave identically. Defined inline in
    // Os/Cpu.hpp, which callers must include.
    // ------------------------------------------------------------------

    //! \brief initialize the singleton
    static inline void init();

    //! \brief return the singleton of the configured Os::Cpu
    //! \return reference to the singleton
    static inline Os::Cpu& getSingleton();

    //! \brief Request the count of the CPUs detected by the system
    //!
    //! This method wraps a singleton implementation.
    //!
    //! \param cpu_count: (output) filled with CPU count on system
    //! \return: OP_OK with valid CPU count, ERROR when error occurs
    //!
    static inline Status getCount(FwSizeType& cpu_count);

    //! \brief Get the CPU tick information for a given CPU
    //!
    //! CPU ticks represent a small time slice of processor time. This will retrieve the used CPU ticks and total
    //! ticks for a given CPU. This information in a running accumulation and thus a sample-to-sample
    //! differencing is needed to see the 'realtime' changing load. This shall be done by the caller. This method wraps
    //! a singleton implementation.
    //!
    //! \param ticks: (output) filled with the tick information for the given CPU
    //! \param cpu_index: index for CPU to read. Default: 0
    //! \return:  ERROR when error occurs, OK otherwise.
    //!
    static inline Status getTicks(Ticks& ticks, FwSizeType cpu_index);
};
}  // namespace Os
#endif  // OS_CPUINTERFACE_HPP_
