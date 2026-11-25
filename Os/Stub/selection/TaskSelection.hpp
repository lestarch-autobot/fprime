// ======================================================================
// \title config/TaskSelection.hpp
// \brief selection of Stub::Task::StubTask as the Os::Task implementation
//
// Supplied as one of the CONFIGURATION_HEADERS of the implementation module: the build system places this file at
// config/TaskSelection.hpp when that implementation is chosen (CHOOSES_IMPLEMENTATIONS in register_fprime_config).
// Os/Task.hpp then defines Os::Task as Os::TaskCommon<Os::Selected::Task>.
// ======================================================================
#ifndef CONFIG_TASKSELECTION_HPP
#define CONFIG_TASKSELECTION_HPP

#include <Os/Stub/Task.hpp>

namespace Os {
namespace Selected {
using Task = Stub::Task::StubTask;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_TASKSELECTION_HPP
