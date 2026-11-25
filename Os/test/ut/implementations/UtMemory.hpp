// ======================================================================
// \title Os/test/ut/implementations/UtMemory.hpp
// \brief TEST ONLY: Os::Memory implementation selected in unit test builds
// ======================================================================
#ifndef OS_TEST_UT_IMPLEMENTATIONS_UTMEMORY_HPP_
#define OS_TEST_UT_IMPLEMENTATIONS_UTMEMORY_HPP_

#include <Os/Interfaces/MemoryInterface.hpp>
#include "config/MemorySelection.hpp"

namespace Os {
namespace Ut {
namespace Memory {

//! \brief TEST ONLY: wraps the selected Os::Memory implementation so that unit tests may intercept its calls
//!
//! Each call is forwarded to the installed `Os::Test::MemoryTestInterceptor`, or directly to the wrapped
//! implementation (`Os::Selected::Memory`) when no interceptor is installed.
class UtMemory : public MemoryInterface {
  public:
    UtMemory();
    ~UtMemory() override;

    //! \brief assignment operator is forbidden
    MemoryInterface& operator=(const MemoryInterface& other) override = delete;

    Status _getUsage(Usage& memory_usage) override;
    MemoryHandle* getHandle() override;

  private:
    Os::Selected::Memory m_real;  //!< Implementation selected for non-test builds
};

}  // namespace Memory
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_IMPLEMENTATIONS_UTMEMORY_HPP_
