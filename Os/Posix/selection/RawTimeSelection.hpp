// ======================================================================
// \title config/RawTimeSelection.hpp
// \brief selection of Posix::RawTime::PosixRawTime as the Os::RawTime implementation
//
// Supplied as one of the CONFIGURATION_HEADERS of the implementation module: the build system places this file at
// config/RawTimeSelection.hpp when that implementation is chosen (CHOOSES_IMPLEMENTATIONS in register_fprime_config).
// Os/RawTime.hpp then defines Os::RawTime as Os::RawTimeCommon<Os::Selected::RawTime>.
// ======================================================================
#ifndef CONFIG_RAWTIMESELECTION_HPP
#define CONFIG_RAWTIMESELECTION_HPP

#include <Os/Posix/RawTime.hpp>

namespace Os {
namespace Selected {
using RawTime = Posix::RawTime::PosixRawTime;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_RAWTIMESELECTION_HPP
