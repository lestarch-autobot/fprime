// ======================================================================
// \title config/FileSelection.hpp
// \brief configured selection of the Os::File implementation
//
// Selects, at compile time, the implementation wrapped by Os::File: include the header defining the implementation and
// name it Os::Selected::File. Os/File.hpp defines Os::File as Os::FileCommon<Os::Selected::File>, which holds the
// implementation by value and forwards to it without virtual dispatch.
//
// The implementation header must not include Os/File.hpp (it derives from Os/Interfaces/FileInterface.hpp). The alias
// only changes the C++ type; the module providing the implementation must still be in the link.
// ======================================================================
#ifndef CONFIG_FILESELECTION_HPP
#define CONFIG_FILESELECTION_HPP

#include <Os/Posix/File.hpp>

namespace Os {
namespace Selected {
using File = Posix::File::PosixFile;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_FILESELECTION_HPP
