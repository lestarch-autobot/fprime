// ======================================================================
// \title Os/test/ut/interceptors/UtSelection.hpp
// \brief TEST ONLY: selects the intercepting Os implementations in unit test builds
//
// Included by config/OsSelection.hpp in unit test builds (BUILD_UT) after it defines the Os::Selected::<Service>
// aliases and the OS_SELECTED_<SERVICE>_HEADER macros. Defines each Os::<Service> as the common class wrapping the
// Ut<Service> implementation, which in turn wraps Os::Selected::<Service> and lets tests intercept its calls.
//
// As with config/OsSelection.hpp, this header MUST NOT include any Os OSAL headers.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_UTSELECTION_HPP_
#define OS_TEST_UT_INTERCEPTORS_UTSELECTION_HPP_

namespace Os {

namespace Ut {
namespace Console {
class UtConsole;
}  // namespace Console
namespace CountingSemaphore {
class UtCountingSemaphore;
}  // namespace CountingSemaphore
namespace Cpu {
class UtCpu;
}  // namespace Cpu
namespace Memory {
class UtMemory;
}  // namespace Memory
namespace Directory {
class UtDirectory;
}  // namespace Directory
namespace FileSystem {
class UtFileSystem;
}  // namespace FileSystem
namespace Mutex {
class UtConditionVariable;
class UtMutex;
}  // namespace Mutex
namespace Queue {
class UtQueue;
}  // namespace Queue
namespace Task {
class UtTask;
}  // namespace Task
namespace File {
class UtFile;
}  // namespace File
namespace RawTime {
class UtRawTime;
}  // namespace RawTime
}  // namespace Ut

using Console = ConsoleCommon<Ut::Console::UtConsole>;
using CountingSemaphore = CountingSemaphoreCommon<Ut::CountingSemaphore::UtCountingSemaphore>;
using Cpu = CpuCommon<Ut::Cpu::UtCpu>;
using Memory = MemoryCommon<Ut::Memory::UtMemory>;
using Directory = DirectoryCommon<Ut::Directory::UtDirectory>;
using FileSystem = FileSystemCommon<Ut::FileSystem::UtFileSystem>;
using ConditionVariable = ConditionVariableCommon<Ut::Mutex::UtConditionVariable>;
using Mutex = MutexCommon<Ut::Mutex::UtMutex>;
using Queue = QueueCommon<Ut::Queue::UtQueue>;
using Task = TaskCommon<Ut::Task::UtTask>;
using File = FileCommon<Ut::File::UtFile>;
using RawTime = RawTimeCommon<Ut::RawTime::UtRawTime>;

}  // namespace Os

#define OS_CONSOLE_HEADER <Os/test/ut/interceptors/UtConsole.hpp>
#define OS_COUNTING_SEMAPHORE_HEADER <Os/test/ut/interceptors/UtCountingSemaphore.hpp>
#define OS_CPU_HEADER <Os/test/ut/interceptors/UtCpu.hpp>
#define OS_MEMORY_HEADER <Os/test/ut/interceptors/UtMemory.hpp>
#define OS_DIRECTORY_HEADER <Os/test/ut/interceptors/UtDirectory.hpp>
#define OS_FILE_SYSTEM_HEADER <Os/test/ut/interceptors/UtFileSystem.hpp>
#define OS_CONDITION_VARIABLE_HEADER <Os/test/ut/interceptors/UtConditionVariable.hpp>
#define OS_MUTEX_HEADER <Os/test/ut/interceptors/UtMutex.hpp>
#define OS_QUEUE_HEADER <Os/test/ut/interceptors/UtQueue.hpp>
#define OS_TASK_HEADER <Os/test/ut/interceptors/UtTask.hpp>
#define OS_FILE_HEADER <Os/test/ut/interceptors/UtFile.hpp>
#define OS_RAW_TIME_HEADER <Os/test/ut/interceptors/UtRawTime.hpp>

#endif  // OS_TEST_UT_INTERCEPTORS_UTSELECTION_HPP_
