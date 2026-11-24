// ======================================================================
// \title config/DirectorySelection.hpp
// \brief configured selection of the Os::Directory implementation
//
// Selects, at compile time, the implementation wrapped by Os::Directory: include the header defining the implementation
// and name it Os::Selected::Directory. Os/Directory.hpp defines Os::Directory as
// Os::DirectoryCommon<Os::Selected::Directory>, which holds the implementation by value and forwards to it without
// virtual dispatch.
//
// The implementation header must not include Os/Directory.hpp (it derives from Os/Interfaces/DirectoryInterface.hpp).
// The alias only changes the C++ type; the module providing the implementation must still be in the link.
// ======================================================================
#ifndef CONFIG_DIRECTORYSELECTION_HPP
#define CONFIG_DIRECTORYSELECTION_HPP

#include <Os/Posix/Directory.hpp>

namespace Os {
namespace Selected {
using Directory = Posix::Directory::PosixDirectory;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_DIRECTORYSELECTION_HPP
