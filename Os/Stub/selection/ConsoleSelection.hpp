// ======================================================================
// \title config/ConsoleSelection.hpp
// \brief selection of Stub::Console::StubConsole as the Os::Console implementation
//
// Supplied as one of the CONFIGURATION_HEADERS of the implementation module: the build system places this file at
// config/ConsoleSelection.hpp when that implementation is chosen (CHOOSES_IMPLEMENTATIONS in register_fprime_config).
// Os/Console.hpp then defines Os::Console as Os::ConsoleCommon<Os::Selected::Console>.
// ======================================================================
#ifndef CONFIG_CONSOLESELECTION_HPP
#define CONFIG_CONSOLESELECTION_HPP

#include <Os/Stub/Console.hpp>

namespace Os {
namespace Selected {
using Console = Stub::Console::StubConsole;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_CONSOLESELECTION_HPP
