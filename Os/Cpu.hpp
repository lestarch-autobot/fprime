// ======================================================================
// \title Os/Cpu.hpp
// \brief common function definitions for Os::Cpu
// ======================================================================
#include <Os/Interfaces/CpuUserInterface.hpp>
#include "Fw/Types/Assert.hpp"
#include "Os/Os.hpp"
#include "config/CpuSelection.hpp"
#ifdef BUILD_UT
#include <Os/test/ut/implementations/UtCpu.hpp>
#endif

#ifndef OS_CPU_HPP_
#define OS_CPU_HPP_

namespace Os {

template <typename T>
class CpuCommon;

#ifdef BUILD_UT
//! \brief Os::Cpu in unit test builds: wraps the implementation selected in config/CpuSelection.hpp in the test-only
//! UtCpu, so that tests may intercept its calls
using Cpu = CpuCommon<Ut::Cpu::UtCpu>;
#else
//! \brief Os::Cpu: the common class on the implementation selected in config/CpuSelection.hpp
using Cpu = CpuCommon<Selected::Cpu>;
#endif

//! \brief cpu implementation
template <typename T>
class CpuCommon final : public CpuUserInterface {
  public:
    //! \brief default constructor
    CpuCommon();

    //! \brief default virtual destructor
    ~CpuCommon();

    //! \brief copy constructor is forbidden
    CpuCommon(const CpuCommon& other) = delete;

    //! \brief copy constructor is forbidden
    CpuCommon(const CpuCommon* other) = delete;

    //! \brief assignment operator is forbidden
    CpuCommon& operator=(const CpuCommon& other) = delete;

    //-----------------------------------------------------------------------------
    // Interface methods
    //-----------------------------------------------------------------------------

    //! \brief initialize the singleton
    static void init();

    //! \brief return singleton
    static CpuCommon& getSingleton();

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
    Status _getCount(FwSizeType& cpu_count);

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
    Status _getTicks(Ticks& ticks, FwSizeType cpu_index);

    //! \brief return the underlying cpu handle (implementation specific).
    //! \return internal task handle representation
    CpuHandle* getHandle();

    //-----------------------------------------------------------------------------
    // Static interface (singleton) methods
    //-----------------------------------------------------------------------------

    //! \brief Request the count of the CPUs detected by the system
    //!
    //! This method wraps a singleton implementation.
    //!
    //! \param cpu_count: (output) filled with CPU count on system
    //! \return: OP_OK with valid CPU count, ERROR when error occurs
    //!
    static Status getCount(FwSizeType& cpu_count);

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
    static Status getTicks(Ticks& ticks, FwSizeType cpu_index);

  private:
    // This section is used to store the implementation-defined file handle. To Os::File and fprime, this type is
    // opaque and thus normal allocation cannot be done. Instead, we allow the implementor to store then handle in
    // the byte-array here and set `handle` to that address for storage.
    T m_delegate;  //!< Delegate for the real implementation
};

}  // namespace Os
namespace Os {

template <typename T>
CpuCommon<T>::CpuCommon() : m_delegate() {}

template <typename T>
CpuCommon<T>::~CpuCommon() {}

template <typename T>
void CpuCommon<T>::init() {
    (void)CpuCommon::getSingleton();
}

template <typename T>
CpuCommon<T>& CpuCommon<T>::getSingleton() {
    static CpuCommon _singleton;
    return _singleton;
}

template <typename T>
typename CpuCommon<T>::Status CpuCommon<T>::_getCount(FwSizeType& cpu_count) {
    return this->m_delegate._getCount(cpu_count);
}

template <typename T>
typename CpuCommon<T>::Status CpuCommon<T>::_getTicks(Ticks& ticks, FwSizeType cpu_index) {
    return this->m_delegate._getTicks(ticks, cpu_index);
}

template <typename T>
CpuHandle* CpuCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}

template <typename T>
typename CpuCommon<T>::Status CpuCommon<T>::getCount(FwSizeType& cpu_count) {
    return CpuCommon::getSingleton()._getCount(cpu_count);
}

template <typename T>
typename CpuCommon<T>::Status CpuCommon<T>::getTicks(Ticks& ticks, FwSizeType cpu_index) {
    return CpuCommon::getSingleton()._getTicks(ticks, cpu_index);
}
}  // namespace Os

#endif  // OS_CONDITION_HPP_
