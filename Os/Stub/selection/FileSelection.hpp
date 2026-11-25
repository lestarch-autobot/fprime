// ======================================================================
// \title config/FileSelection.hpp
// \brief selection of Stub::File::StubFile as the Os::File implementation
//
// Supplied as one of the CONFIGURATION_HEADERS of the implementation module: the build system places this file at
// config/FileSelection.hpp when that implementation is chosen (CHOOSES_IMPLEMENTATIONS in register_fprime_config).
// Os/File.hpp then defines Os::File as Os::FileCommon<Os::Selected::File>.
// ======================================================================
#ifndef CONFIG_FILESELECTION_HPP
#define CONFIG_FILESELECTION_HPP

#include <Os/Stub/File.hpp>

namespace Os {
namespace Selected {
using File = Stub::File::StubFile;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_FILESELECTION_HPP
