// ======================================================================
// \title config/RawTimeSelection.hpp
// \brief configured selection of the Os::RawTime implementation
//
// Selects, at compile time, the implementation wrapped by Os::RawTime: include the header defining the implementation
// and name it Os::Selected::RawTime. Os/RawTime.hpp defines Os::RawTime as Os::RawTimeCommon<Os::Selected::RawTime>,
// which holds the implementation by value and forwards to it without virtual dispatch.
//
// The implementation header must not include Os/RawTime.hpp (it derives from Os/Interfaces/RawTimeInterface.hpp). The
// alias only changes the C++ type; the module providing the implementation must still be in the link.
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
