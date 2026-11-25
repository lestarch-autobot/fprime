// ======================================================================
// \title config/FileSystemSelection.hpp
// \brief selection of Posix::FileSystem::PosixFileSystem as the Os::FileSystem implementation
//
// Supplied as one of the CONFIGURATION_HEADERS of the implementation module: the build system places this file at
// config/FileSystemSelection.hpp when that implementation is chosen (CHOOSES_IMPLEMENTATIONS in
// register_fprime_config). Os/FileSystem.hpp then defines Os::FileSystem as
// Os::FileSystemCommon<Os::Selected::FileSystem>.
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
