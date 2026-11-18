// ======================================================================
// \title Os/Task.hpp
// \brief public Os::Task interface and alias
//
// This header aggregates all definitions needed to use Os::Task: the
// interface, the configured alias, and the concrete type the alias names.
//
// The includes below must appear in the order given (see Os/Mutex.hpp for the
// full rationale):
//
//   1. Os/TaskInterface.hpp includes config/OsSelection.hpp, which defines the
//      Os::Task alias and OS_TASK_HEADER, then defines TaskHandle,
//      TaskInterface, and TaskRegistry.
//
//   2. OS_TASK_HEADER points to the header of the class Os::Task names:
//      Os/DelegateTask.hpp for link-time selection, or the header of a concrete
//      implementation (e.g. Os/Posix/Task.hpp) for compile-time selection.
// ======================================================================
#ifndef Os_Task_hpp_
#define Os_Task_hpp_

#include "Os/TaskInterface.hpp"

// Validate that OS_TASK_HEADER was defined by config/OsSelection.hpp
#ifndef OS_TASK_HEADER
#error "OS_TASK_HEADER must be defined in config/OsSelection.hpp"
#endif

#include OS_TASK_HEADER

#endif  // Os_Task_hpp_
