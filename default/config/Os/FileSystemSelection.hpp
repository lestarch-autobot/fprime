// ======================================================================
// \title config/FileSystemSelection.hpp
// \brief configured selection of the Os::FileSystem implementation
//
// Selects, at compile time, the implementation wrapped by Os::FileSystem: include the header defining the
// implementation and name it Os::Selected::FileSystem. Os/FileSystem.hpp defines Os::FileSystem as
// Os::FileSystemCommon<Os::Selected::FileSystem>, which holds the implementation by value and forwards to it without
// virtual dispatch.
//
// The implementation header must not include Os/FileSystem.hpp (it derives from Os/Interfaces/FileSystemInterface.hpp).
// The alias only changes the C++ type; the module providing the implementation must still be in the link.
// ======================================================================
#ifndef CONFIG_FILESYSTEMSELECTION_HPP
#define CONFIG_FILESYSTEMSELECTION_HPP

#include <Os/Posix/FileSystem.hpp>

namespace Os {
namespace Selected {
using FileSystem = Posix::FileSystem::PosixFileSystem;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_FILESYSTEMSELECTION_HPP
