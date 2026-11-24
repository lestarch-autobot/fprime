// ======================================================================
// \title config/ConsoleSelection.hpp
// \brief configured selection of the Os::Console implementation
//
// Selects, at compile time, the implementation wrapped by Os::Console: include the header defining the implementation
// and name it Os::Selected::Console. Os/Console.hpp defines Os::Console as Os::ConsoleCommon<Os::Selected::Console>,
// which holds the implementation by value and forwards to it without virtual dispatch.
//
// The implementation header must not include Os/Console.hpp (it derives from Os/Interfaces/ConsoleInterface.hpp). The
// alias only changes the C++ type; the module providing the implementation must still be in the link.
// ======================================================================
#ifndef CONFIG_CONSOLESELECTION_HPP
#define CONFIG_CONSOLESELECTION_HPP

#include <Os/Posix/Console.hpp>

namespace Os {
namespace Selected {
using Console = Posix::Console::PosixConsole;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_CONSOLESELECTION_HPP
