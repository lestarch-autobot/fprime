// ======================================================================
// \title config/DirectorySelection.hpp
// \brief selection of Stub::Directory::StubDirectory as the Os::Directory implementation
//
// Supplied as one of the CONFIGURATION_HEADERS of the implementation module: the build system places this file at
// config/DirectorySelection.hpp when that implementation is chosen (CHOOSES_IMPLEMENTATIONS in register_fprime_config).
// Os/Directory.hpp then defines Os::Directory as Os::DirectoryCommon<Os::Selected::Directory>.
// ======================================================================
#ifndef CONFIG_DIRECTORYSELECTION_HPP
#define CONFIG_DIRECTORYSELECTION_HPP

#include <Os/Stub/Directory.hpp>

namespace Os {
namespace Selected {
using Directory = Stub::Directory::StubDirectory;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_DIRECTORYSELECTION_HPP
