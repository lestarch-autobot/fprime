// ======================================================================
// \title Os/Interfaces/CpuInterface.hpp
// \brief pure-virtual interface implemented by Os::Cpu implementations
// ======================================================================
#ifndef OS_INTERFACES_CPUINTERFACE_HPP_
#define OS_INTERFACES_CPUINTERFACE_HPP_

#include <Os/Interfaces/CpuUserInterface.hpp>

namespace Os {

//! \brief interface for cpu implementation
class CpuInterface : public CpuUserInterface {
  public:
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
};

}  // namespace Os

#endif  // OS_INTERFACES_CPUINTERFACE_HPP_
