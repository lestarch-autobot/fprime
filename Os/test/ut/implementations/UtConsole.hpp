// ======================================================================
// \title Os/test/ut/implementations/UtConsole.hpp
// \brief TEST ONLY: Os::Console implementation selected in unit test builds
// ======================================================================
// Included before the guard: Os/Console.hpp includes this file, and may be reached from the interface header
#include <Os/Interfaces/ConsoleInterface.hpp>
#ifndef OS_TEST_UT_IMPLEMENTATIONS_UTCONSOLE_HPP_
#define OS_TEST_UT_IMPLEMENTATIONS_UTCONSOLE_HPP_

#ifndef OS_SELECTED_CONSOLE_HEADER
#error "OS_SELECTED_CONSOLE_HEADER must be defined in config/OsSelection.hpp"
#endif
#include OS_SELECTED_CONSOLE_HEADER

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
