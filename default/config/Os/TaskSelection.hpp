// ======================================================================
// \title config/TaskSelection.hpp
// \brief configured selection of the Os::Task implementation
//
// Selects, at compile time, the implementation wrapped by Os::Task: include the header defining the implementation and
// name it Os::Selected::Task. Os/Task.hpp defines Os::Task as Os::TaskCommon<Os::Selected::Task>, which holds the
// implementation by value and forwards to it without virtual dispatch.
//
// The implementation header must not include Os/Task.hpp (it derives from Os/Interfaces/TaskInterface.hpp). The alias
// only changes the C++ type; the module providing the implementation must still be in the link.
// ======================================================================
#ifndef CONFIG_TASKSELECTION_HPP
#define CONFIG_TASKSELECTION_HPP

#include <Os/Posix/Task.hpp>

namespace Os {
namespace Selected {
using Task = Posix::Task::PosixTask;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_TASKSELECTION_HPP
