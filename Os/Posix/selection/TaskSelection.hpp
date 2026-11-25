// ======================================================================
// \title config/TaskSelection.hpp
// \brief selection of Posix::Task::PosixTask as the Os::Task implementation
//
// Supplied as one of the CONFIGURATION_HEADERS of the implementation module: the build system places this file at
// config/TaskSelection.hpp when that implementation is chosen (CHOOSES_IMPLEMENTATIONS in register_fprime_config).
// Os/Task.hpp then defines Os::Task as Os::TaskCommon<Os::Selected::Task>.
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
