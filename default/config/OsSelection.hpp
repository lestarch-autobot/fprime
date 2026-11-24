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
//   - forward-declare the implementation and define the Os::<Service> alias
//   - define OS_<SERVICE>_HEADER as the header defining the implementation
//
// IMPORTANT: CIRCULAR DEPENDENCY PREVENTION
//
//   - This header MUST NOT include any Os OSAL headers (Os/*.hpp).
//   - Only forward-declare types and define the Os::<Service> aliases.
//   - Each OS_*_HEADER is included by the public Os/<Service>.hpp header
//     after it defines Os::<Service>Common.
//
// IMPORTANT: a project override of this header replaces it entirely, so the
// override must define ALL aliases and OS_*_HEADER macros below, not just the
// service being changed.
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
// Unit test builds (BUILD_UT) wrap the File and RawTime selections in
// Os::Ut::File::UtFile and Os::Ut::RawTime::UtRawTime so that tests may
// intercept calls (see Os/Interfaces/Test).
// ======================================================================
#ifndef CONFIG_OSSELECTION_HPP
#define CONFIG_OSSELECTION_HPP

// Compile-time selection of the implementations wrapped by the Os::<Service>Common classes
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
using Console = ConsoleCommon<Posix::Console::PosixConsole>;
using Task = TaskCommon<Posix::Task::PosixTask>;
#ifdef BUILD_UT
// Unit test builds wrap the selection above in UtFile so that tests may intercept file calls
namespace Ut {
namespace File {
class UtFile;
using RealFile = Posix::File::PosixFile;
}  // namespace File
}  // namespace Ut
using File = FileCommon<Ut::File::UtFile>;
#else
using File = FileCommon<Posix::File::PosixFile>;
#endif
using FileSystem = FileSystemCommon<Posix::FileSystem::PosixFileSystem>;
using Directory = DirectoryCommon<Posix::Directory::PosixDirectory>;
using Queue = QueueCommon<Generic::PriorityQueue>;
using Mutex = MutexCommon<Posix::Mutex::PosixMutex>;
using ConditionVariable = ConditionVariableCommon<Posix::Mutex::PosixConditionVariable>;
#ifdef BUILD_UT
// Unit test builds wrap the selection in UtRawTime so that tests may intercept raw time calls
namespace Ut {
namespace RawTime {
class UtRawTime;
using RealRawTime = Posix::RawTime::PosixRawTime;
}  // namespace RawTime
}  // namespace Ut
using RawTime = RawTimeCommon<Ut::RawTime::UtRawTime>;
#else
using RawTime = RawTimeCommon<Posix::RawTime::PosixRawTime>;
#endif

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
using CountingSemaphore = CountingSemaphoreCommon<Posix::Semaphore::PosixCountingSemaphore>;
using Cpu = CpuCommon<Linux::Cpu::LinuxCpu>;
using Memory = MemoryCommon<Linux::Memory::LinuxMemory>;
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
using CountingSemaphore = CountingSemaphoreCommon<Darwin::Semaphore::DarwinCountingSemaphore>;
using Cpu = CpuCommon<Darwin::Cpu::DarwinCpu>;
using Memory = MemoryCommon<Darwin::Memory::DarwinMemory>;
#else
#error "No default Os implementations for this platform: override config/OsSelection.hpp"
#endif

}  // namespace Os

#define OS_CONSOLE_HEADER <Os/Posix/Console.hpp>
#define OS_TASK_HEADER <Os/Posix/Task.hpp>
#ifdef BUILD_UT
#define OS_FILE_HEADER <Os/Ut/File.hpp>
#define OS_UT_REAL_FILE_HEADER <Os/Posix/File.hpp>
#else
#define OS_FILE_HEADER <Os/Posix/File.hpp>
#endif
#define OS_FILE_SYSTEM_HEADER <Os/Posix/FileSystem.hpp>
#define OS_DIRECTORY_HEADER <Os/Posix/Directory.hpp>
#define OS_QUEUE_HEADER <Os/Generic/PriorityQueue.hpp>
#if defined(TGT_OS_TYPE_LINUX)
#define OS_COUNTING_SEMAPHORE_HEADER <Os/Posix/CountingSemaphore.hpp>
#define OS_CPU_HEADER <Os/Linux/Cpu.hpp>
#define OS_MEMORY_HEADER <Os/Linux/Memory.hpp>
#elif defined(TGT_OS_TYPE_DARWIN)
#define OS_COUNTING_SEMAPHORE_HEADER <Os/Darwin/CountingSemaphore.hpp>
#define OS_CPU_HEADER <Os/Darwin/Cpu.hpp>
#define OS_MEMORY_HEADER <Os/Darwin/Memory.hpp>
#endif

#ifdef BUILD_UT
#define OS_RAW_TIME_HEADER <Os/Ut/RawTime.hpp>
#define OS_UT_REAL_RAW_TIME_HEADER <Os/Posix/RawTime.hpp>
#else
#define OS_RAW_TIME_HEADER <Os/Posix/RawTime.hpp>
#endif
#define OS_MUTEX_HEADER <Os/Posix/Mutex.hpp>
#define OS_CONDITION_VARIABLE_HEADER <Os/Posix/ConditionVariable.hpp>

#endif  // CONFIG_OSSELECTION_HPP
