// ======================================================================
// \title Os/test/ut/implementations/UtConsole.hpp
// \brief TEST ONLY: Os::Console implementation selected in unit test builds
// ======================================================================
#ifndef OS_TEST_UT_IMPLEMENTATIONS_UTCONSOLE_HPP_
#define OS_TEST_UT_IMPLEMENTATIONS_UTCONSOLE_HPP_

#include <Os/Interfaces/ConsoleInterface.hpp>
#include "config/ConsoleSelection.hpp"

namespace Os {
namespace Ut {
namespace Console {

//! \brief TEST ONLY: wraps the selected Os::Console implementation so that unit tests may intercept its calls
//!
//! Each call is forwarded to the installed `Os::Test::ConsoleTestInterceptor`, or directly to the wrapped
//! implementation (`Os::Selected::Console`) when no interceptor is installed.
class UtConsole : public ConsoleInterface {
  public:
    UtConsole();
    UtConsole(const UtConsole& other);
    UtConsole& operator=(const UtConsole& other);
    ~UtConsole() override;

    void writeMessage(const CHAR* message, const FwSizeType size) override;
    ConsoleHandle* getHandle() override;

  private:
    Os::Selected::Console m_real;  //!< Implementation selected for non-test builds
};

}  // namespace Console
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_IMPLEMENTATIONS_UTCONSOLE_HPP_
