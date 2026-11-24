// ======================================================================
// \title config/OsSelection.hpp
// \brief configured selection of Os OSAL implementations
//
// This header selects, at compile time, the implementation wrapped by each
// Os::<Service>Common class: Os::<Service> is an alias of
// Os::<Service>Common<T>, which holds the implementation T by value and
// forwards to it without virtual dispatch.
//
// For each service this header must:
//
//   - forward-declare the implementation and define the Os::Selected::<Service>
//     alias naming it
//   - define OS_SELECTED_<SERVICE>_HEADER as the header defining the
//     implementation
//
// The Os::<Service> aliases and OS_<SERVICE>_HEADER macros are then derived from
// these at the bottom of this header and need not change in an override.
//
// IMPORTANT: CIRCULAR DEPENDENCY PREVENTION
//
//   - This header MUST NOT include any Os OSAL headers (Os/*.hpp).
//   - Only forward-declare types and define the aliases.
//   - Each OS_<SERVICE>_HEADER is included by the public Os/<Service>.hpp
//     header after it defines Os::<Service>Common.
//
// IMPORTANT: a project override of this header replaces it entirely, so the
// override must define ALL aliases and macros below, not just the service being
// changed.
//
// Mutex and ConditionVariable: a condition variable implementation operates on
// the handle of the selected Os::Mutex (e.g. Os/Posix/ConditionVariable.cpp
// casts Os::Mutex::getHandle() to PosixMutexHandle), so the two MUST be
// selected as a compatible pair. Pairing a Mutex with a ConditionVariable that
// expects a different handle type is undefined behavior that the compiler
// cannot detect. A ConditionVariable that does not inspect the mutex handle
// (e.g. Os::Stub::Mutex::StubConditionVariable) is compatible with any Mutex.
//
// The alias only changes the C++ type; the module providing the implementation
// must still be in the link (via CHOOSES_IMPLEMENTATIONS for an in-tree
// implementation, or DEPENDS for a project module).
//
// Unit test builds (BUILD_UT) wrap every selection in a test-only
// Os::Ut::<Service>::Ut<Service> implementation so that tests may intercept
// calls (see Os/test/ut/interceptors). Os::Selected::<Service> always names the
// implementation selected here.
// ======================================================================
#ifndef CONFIG_OSSELECTION_HPP
#define CONFIG_OSSELECTION_HPP

namespace Os {

template <typename T>
class ConsoleCommon;
template <typename T>
class TaskCommon;
template <typename T>
class FileCommon;
template <typename T>
class FileSystemCommon;
template <typename T>
class DirectoryCommon;
template <typename T>
class QueueCommon;
template <typename T>
class CountingSemaphoreCommon;
template <typename T>
class CpuCommon;
template <typename T>
class MemoryCommon;
template <typename T>
class RawTimeCommon;
template <typename T>
class MutexCommon;
template <typename T>
class ConditionVariableCommon;

// Forward declarations of the selected implementations
namespace Posix {
namespace Console {
class PosixConsole;
}
namespace Task {
class PosixTask;
}
namespace File {
class PosixFile;
}
namespace FileSystem {
class PosixFileSystem;
}
namespace Directory {
class PosixDirectory;
}
namespace RawTime {
class PosixRawTime;
}
namespace Mutex {
class PosixMutex;
class PosixConditionVariable;
}  // namespace Mutex
}  // namespace Posix
namespace Generic {
class PriorityQueue;
}
#if defined(TGT_OS_TYPE_LINUX)
namespace Posix {
namespace Semaphore {
class PosixCountingSemaphore;
}
}  // namespace Posix
namespace Linux {
namespace Cpu {
class LinuxCpu;
}
namespace Memory {
class LinuxMemory;
}
}  // namespace Linux
#elif defined(TGT_OS_TYPE_DARWIN)
namespace Darwin {
namespace Semaphore {
class DarwinCountingSemaphore;
}
namespace Cpu {
class DarwinCpu;
}
namespace Memory {
class DarwinMemory;
}
}  // namespace Darwin
#else
#error "No default Os implementations for this platform: override config/OsSelection.hpp"
#endif

// Compile-time selection of the implementation of each service
namespace Selected {
using Console = Posix::Console::PosixConsole;
using Task = Posix::Task::PosixTask;
using File = Posix::File::PosixFile;
using FileSystem = Posix::FileSystem::PosixFileSystem;
using Directory = Posix::Directory::PosixDirectory;
using Queue = Generic::PriorityQueue;
using Mutex = Posix::Mutex::PosixMutex;
using ConditionVariable = Posix::Mutex::PosixConditionVariable;
using RawTime = Posix::RawTime::PosixRawTime;
#if defined(TGT_OS_TYPE_LINUX)
using CountingSemaphore = Posix::Semaphore::PosixCountingSemaphore;
using Cpu = Linux::Cpu::LinuxCpu;
using Memory = Linux::Memory::LinuxMemory;
#elif defined(TGT_OS_TYPE_DARWIN)
using CountingSemaphore = Darwin::Semaphore::DarwinCountingSemaphore;
using Cpu = Darwin::Cpu::DarwinCpu;
using Memory = Darwin::Memory::DarwinMemory;
#endif
}  // namespace Selected

}  // namespace Os

// Headers defining the selected implementations
#define OS_SELECTED_CONSOLE_HEADER <Os/Posix/Console.hpp>
#define OS_SELECTED_TASK_HEADER <Os/Posix/Task.hpp>
#define OS_SELECTED_FILE_HEADER <Os/Posix/File.hpp>
#define OS_SELECTED_FILE_SYSTEM_HEADER <Os/Posix/FileSystem.hpp>
#define OS_SELECTED_DIRECTORY_HEADER <Os/Posix/Directory.hpp>
#define OS_SELECTED_QUEUE_HEADER <Os/Generic/PriorityQueue.hpp>
#define OS_SELECTED_MUTEX_HEADER <Os/Posix/Mutex.hpp>
#define OS_SELECTED_CONDITION_VARIABLE_HEADER <Os/Posix/ConditionVariable.hpp>
#define OS_SELECTED_RAW_TIME_HEADER <Os/Posix/RawTime.hpp>
#if defined(TGT_OS_TYPE_LINUX)
#define OS_SELECTED_COUNTING_SEMAPHORE_HEADER <Os/Posix/CountingSemaphore.hpp>
#define OS_SELECTED_CPU_HEADER <Os/Linux/Cpu.hpp>
#define OS_SELECTED_MEMORY_HEADER <Os/Linux/Memory.hpp>
#elif defined(TGT_OS_TYPE_DARWIN)
#define OS_SELECTED_COUNTING_SEMAPHORE_HEADER <Os/Darwin/CountingSemaphore.hpp>
#define OS_SELECTED_CPU_HEADER <Os/Darwin/Cpu.hpp>
#define OS_SELECTED_MEMORY_HEADER <Os/Darwin/Memory.hpp>
#endif

// Os::<Service> aliases and OS_<SERVICE>_HEADER macros derived from the selection above
#ifdef BUILD_UT
// Unit test builds wrap each selection in a test-only implementation so that tests may intercept calls
#include <Os/test/ut/interceptors/UtSelection.hpp>
#else
namespace Os {
using Console = ConsoleCommon<Selected::Console>;
using Task = TaskCommon<Selected::Task>;
using File = FileCommon<Selected::File>;
using FileSystem = FileSystemCommon<Selected::FileSystem>;
using Directory = DirectoryCommon<Selected::Directory>;
using Queue = QueueCommon<Selected::Queue>;
using Mutex = MutexCommon<Selected::Mutex>;
using ConditionVariable = ConditionVariableCommon<Selected::ConditionVariable>;
using RawTime = RawTimeCommon<Selected::RawTime>;
using CountingSemaphore = CountingSemaphoreCommon<Selected::CountingSemaphore>;
using Cpu = CpuCommon<Selected::Cpu>;
using Memory = MemoryCommon<Selected::Memory>;
}  // namespace Os
#define OS_CONSOLE_HEADER OS_SELECTED_CONSOLE_HEADER
#define OS_TASK_HEADER OS_SELECTED_TASK_HEADER
#define OS_FILE_HEADER OS_SELECTED_FILE_HEADER
#define OS_FILE_SYSTEM_HEADER OS_SELECTED_FILE_SYSTEM_HEADER
#define OS_DIRECTORY_HEADER OS_SELECTED_DIRECTORY_HEADER
#define OS_QUEUE_HEADER OS_SELECTED_QUEUE_HEADER
#define OS_MUTEX_HEADER OS_SELECTED_MUTEX_HEADER
#define OS_CONDITION_VARIABLE_HEADER OS_SELECTED_CONDITION_VARIABLE_HEADER
#define OS_RAW_TIME_HEADER OS_SELECTED_RAW_TIME_HEADER
#define OS_COUNTING_SEMAPHORE_HEADER OS_SELECTED_COUNTING_SEMAPHORE_HEADER
#define OS_CPU_HEADER OS_SELECTED_CPU_HEADER
#define OS_MEMORY_HEADER OS_SELECTED_MEMORY_HEADER
#endif

#endif  // CONFIG_OSSELECTION_HPP
