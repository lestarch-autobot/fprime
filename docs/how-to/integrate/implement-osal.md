# Implement an OS Abstraction Layer

This guide provides step-by-step instructions for implementing a new OS Abstraction Layer (OSAL) for F´. The F´ OSAL provides a uniform interface to operating system services, allowing F´ to run on multiple operating systems without modification to the source code. For more information on the architecture and design of the OSAL, refer to the [OSAL Software Design Document](../../../Os/docs/sdd.md).

---

## Prerequisites

Before starting, you should have:

* An understanding of the [OSAL architecture](../../../Os/docs/sdd.md#5-implementation-architecture), specifically the [wrapper pattern](../../../Os/docs/sdd.md#51-wrapper-pattern).
* A working F´ build environment for testing.
* Familiarity with [F´ libraries](../develop/develop-fprime-libraries.md) and [F´ platform files](../develop/develop-fprime-libraries.md#optional-platform-folder-and-platform-files).
* Knowledge of your target OS APIs (e.g., mutex, task creation, file I/O).

---

## Overview

The OSAL uses a wrapper pattern to decouple F´ application code from platform-specific OS calls. Each OSAL _service_ (Mutex, File, Task, etc.) has three layers: an **interface class** defining the contract, a **wrapper class** (e.g., `Os::Mutex`) that application code uses, and a **platform-specific implementation** that the wrapper holds by value and forwards to. The implementation is selected at compile time in the service's `config/<Service>Selection.hpp`. Before proceeding, read the [OSAL Software Design Document](../../../Os/docs/sdd.md) — in particular the [delegate pattern](../../../Os/docs/sdd.md#51-delegate-pattern) and [implementation architecture](../../../Os/docs/sdd.md#5-implementation-architecture) sections — as this guide assumes familiarity with those concepts.

This guide walks through implementing an OSAL for a hypothetical OS called **"MyOs"**, and walks through the implementation of the `Os::Mutex` service as the example. The same process applies for all other OSAL services. Reference implementations are linked in the [Additional Resources](#additional-resources) section below.

---

## Step 1 — Set Up the Library

An OSAL implementation is packaged as an [F´ library](../develop/develop-fprime-libraries.md). Create the following directory structure:

```
fprime-my-os/
├── FprimeMyOs/
│   └── Os/
│       ├── CMakeLists.txt
│       ├── Mutex.hpp
│       ├── Mutex.cpp
└── library.cmake
```

The `library.cmake` file at the root adds the OSAL module directory:

```cmake
# library.cmake
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/FprimeMyOs/Os")
```

> [!IMPORTANT]
> All module directories for libraries are namespaced under `FprimeMyOs/` to avoid collisions with core F´ modules and other libraries.

---

## Step 2 — Implement an OSAL Module

Each OSAL service is implemented through a CMake/C++ module that requires an **implementation class** that inherits from the corresponding interface. The interface header files in `fprime/Os/` define the contract (pure virtual methods) that must be implemented. 

To keep track of state through the OS's native constructs (e.g. a mutex handle, such as `pthread_mutex_t` in Posix), the implementation class contains a **m_handle** member that wraps the native OS primitives.

### 2.1 — Define the Handle and Implementation Class

Create `FprimeMyOs/Os/Mutex.hpp`:

```c++
// FprimeMyOs/Os/Mutex.hpp
#ifndef MYOS_MUTEX_HPP
#define MYOS_MUTEX_HPP

#include <Os/Interfaces/MutexInterface.hpp>  // the pure-virtual interface; Status comes from Os::MutexUserInterface
#include <myos/mutex.h>  // MyOs native mutex API

namespace Os {
namespace MyOs {
namespace Mutex {

// Handle wraps the native OS mutex primitive
struct MyOsMutexHandle : public MutexHandle {
    myos_mutex_t m_mutex;  // MyOs native mutex type
};

// Implementation of MutexInterface using MyOs APIs
class MyOsMutex : public MutexInterface {
  public:
    MyOsMutex();
    ~MyOsMutex() override;

    MutexHandle* getHandle() override;
    Status take() override;
    Status release() override;

  private:
    MyOsMutexHandle m_handle;  // Handle wrapping the native OS mutex
};

}  // namespace Mutex
}  // namespace MyOs
}  // namespace Os
#endif
```

> [!TIP]
> Look at each interface header in `Os/` (e.g., `Os/Interfaces/MutexInterface.hpp`, `Os/Interfaces/FileInterface.hpp`, `Os/Interfaces/TaskInterface.hpp`) to see the exact set of pure virtual methods that need to be implemented for each. Each interface also defines a `Status` enum — your implementation must return the appropriate [status values](../../../Os/docs/sdd.md#24-error-handling). Note: for services that support compile-time selection, the pure-virtual contract lives in the `*Interface.hpp` header (e.g. `Os/MutexInterface.hpp`, `Os/RawTimeInterface.hpp`), while `Os/Mutex.hpp` / `Os/RawTime.hpp` are thin alias/aggregation headers.

### 2.2 — Implement the Methods

Create `FprimeMyOs/Os/Mutex.cpp`:

```c++
// FprimeMyOs/Os/Mutex.cpp
#include <FprimeMyOs/Os/Mutex.hpp>
#include <Fw/Types/Assert.hpp>

namespace Os {
namespace MyOs {
namespace Mutex {

MyOsMutex::MyOsMutex() : Os::MutexInterface(), m_handle() {
    int status = myos_mutex_init(&this->m_handle.m_mutex);
    FW_ASSERT(status == 0, static_cast<FwAssertArgType>(status));
}

MyOsMutex::~MyOsMutex() {
    myos_mutex_destroy(&this->m_handle.m_mutex);
}

MyOsMutex::Status MyOsMutex::take() {
    int status = myos_mutex_lock(&this->m_handle.m_mutex);
    switch (status) {
        case 0:
            return Status::OP_OK;
        case MYOS_EBUSY:
            return Status::ERROR_BUSY;
        case MYOS_EDEADLK:
            return Status::ERROR_DEADLOCK;
        default:
            return Status::ERROR_OTHER;
    }
}

MyOsMutex::Status MyOsMutex::release() {
    int status = myos_mutex_unlock(&this->m_handle.m_mutex);
    // TODO: Map MyOs error codes to appropriate Status values
    return (status == 0) ? Status::OP_OK : Status::ERROR_OTHER;
}

MutexHandle* MyOsMutex::getHandle() {
    return &this->m_handle;
}

}  // namespace Mutex
}  // namespace MyOs
}  // namespace Os
```

> [!CAUTION]
> When an `Os::Mutex` wrapper object is created, the underlying implementation object `MyOsMutex` is constructed in-place using placement new into a fixed-size byte array of size `FW_MUTEX_HANDLE_MAX_SIZE`. This constant must be defined in `PlatformCfg.fpp` in the library configuration. The compiler will help identify the required size based on the `sizeof(MyOsMutex)`.

---

## Step 3 — Supply the Selection Header

Every OSAL service is selected at compile time. `Os::Mutex` is an alias of `Os::MutexCommon<T>`, a wrapper template that holds the implementation `T` by value and forwards each call to it directly: no virtual dispatch, no factory function, and no handle storage. The implementation `T` of each service is named by that service's own configuration header, e.g. `config/MutexSelection.hpp` for Mutex.

A selection header does two things: it includes the header defining the implementation, and names the implementation `Os::Selected::<Service>`. `Os/Mutex.hpp` then defines `Os::Mutex` as `Os::MutexCommon<Os::Selected::Mutex>`.

Each implementation supplies the selection header that selects it, in a `selection` directory next to the implementation. The build system places the header at `config/MutexSelection.hpp` when, and only when, your implementation is chosen (Step 7), so the selected type always matches the implementation that is linked. Projects do not write or override selection headers.

Create `FprimeMyOs/Os/selection/MutexSelection.hpp`:

```c++
// FprimeMyOs/Os/selection/MutexSelection.hpp
#ifndef CONFIG_MUTEXSELECTION_HPP
#define CONFIG_MUTEXSELECTION_HPP

#include <FprimeMyOs/Os/Mutex.hpp>

namespace Os {
namespace Selected {
using Mutex = MyOs::Mutex::MyOsMutex;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_MUTEXSELECTION_HPP
```

> [!WARNING]
> The header is included as `config/MutexSelection.hpp` from the build cache. Never include it by its path in the `selection` directory.

### Important Constraints

- **One Selection per Build**: `Os::Mutex` is a different type for each selection, so the selection is global to the build. An executable, deployment, or unit test cannot choose a different OSAL implementation (see [Implementations with Configuration Headers](../../user-manual/build-system/cmake-implementations.md#implementations-with-configuration-headers)).
- **No Circular Dependencies**: an implementation header must not include the `Os/<Service>.hpp` of its own service; it includes `Os/Interfaces/<Service>Interface.hpp`. It may include other services (e.g. a queue holding an `Os::Mutex`) as long as their selected implementations do not depend back on it.
- **Paired Services**: a ConditionVariable implementation operates on the handle of the selected `Os::Mutex`, so the two are registered, and therefore chosen, together (`register_os_implementation("Mutex;ConditionVariable" ...)`).
- **RawTime**: the implementation must be copyable and constructible from a `RawTimeSource`.
- **Unit Test Builds**: under `BUILD_UT` each `Os/<Service>.hpp` wraps the selection in a test-only `Os::Ut::<Service>::Ut<Service>` implementation so that unit tests can intercept calls (see `Os/test/ut/implementations/` and `Os/test/ut/interceptors/`). A selection header needs nothing for this.

---

## Step 4 — Register the Module in CMake

Create `FprimeMyOs/Os/CMakeLists.txt` and use the `register_os_implementation` function to register the implementation:

```cmake
# FprimeMyOs/Os/CMakeLists.txt
restrict_platforms(MyOs)

register_os_implementation("Mutex" MyOs) # args: "ServiceName" "PlatformSuffix"
```

This single `register_os_implementation` call:

1. Expects the files `Mutex.hpp`, `Mutex.cpp`, and `selection/MutexSelection.hpp` in the current directory.
2. Creates an `Os_Mutex_MyOs_Implementation` target with the implementation sources (`Mutex.cpp`, `Mutex.hpp`).
3. Creates an `Os_Mutex_MyOs` target, registered as an implementation of `Os_Mutex`, that brings the implementation into the link and supplies `selection/MutexSelection.hpp` as its `CONFIGURATION_HEADERS`.

If the module depends on other modules, pass them as additional arguments:

```cmake
register_os_implementation("Task" MyOs MyOs_Shared Fw_Time)
```

Interfaces that are implemented together are listed together:

```cmake
register_os_implementation("Mutex;ConditionVariable" MyOs MyOs_Shared)
register_os_implementation("File;FileSystem;Directory" MyOs MyOs_Shared)
```

The `restrict_platforms(MyOs)` call ensures these implementations only build when the `MyOs` platform is selected.

---

## Step 5 - Unit Tests

Thorough Unit Testing is outside the scope of this How-To guide, but is critical to ensure the correctness of your implementation. F´ provides OS-agnostic unit tests for most OSAL service under `fprime/Os/test/`. These tests can be run against your implementation to validate that it behaves correctly according to the interface contract. You may also add additional tests if desired.

---

## Step 6 — Repeat for All desired OSAL Services

The process described above for `Os::Mutex` is the same for every OSAL module. For each module:

1. Create `<Module>.hpp` with a handle struct and implementation class inheriting from the interface.
2. Create `<Module>.cpp` implementing the virtual methods using your OS's native APIs.
3. Create `selection/<Module>Selection.hpp` selecting it.
4. Register with `register_os_implementation` in `CMakeLists.txt`.

> [!NOTE]
> F´ provides two platform-independent queue implementations: `Os_Generic_PriorityQueue` (mutex-based — the default on most platforms) and `Os_Generic_LocklessPriorityQueue` (lockless, ISR-safe — opt-in for targets that send or receive from interrupt context). You do not need to write an OS-specific queue unless neither generic implementation is suitable for your target.

The full set of [OSAL modules](../../../Os/docs/sdd.md#2-core-services) that can be implemented is:

| Module | Interface | Key Methods |
|---|---|---|
| **Mutex** | `MutexInterface` | `take()`, `release()` |
| **CountingSemaphore** | `CountingSemaphoreInterface` | `wait()`, `waitTimeout()`, `tryWait()`, `post()` |
| **ConditionVariable** | `ConditionVariableInterface` | `pend()`, `notify()`, `notifyAll()` |
| **Task** | `TaskInterface` | `start()`, `join()`, `delay()` |
| **Queue** | `QueueInterface` | `send()`, `receive()` |
| **File** | `FileInterface` | `open()`, `read()`, `write()`, `close()`, `seek()` |
| **FileSystem** | `FileSystemInterface` | `rename()`, `remove()`, `stat()` |
| **Directory** | `DirectoryInterface` | `open()`, `read()`, `close()` |
| **Console** | `ConsoleInterface` | `write()` |
| **RawTime** | `RawTimeInterface` | `now()`, `getTimeInterval()` (constructible from `RawTimeSource`) |
| **Cpu** | `CpuInterface` | `getCount()`, `getTicks()` |
| **Memory** | `MemoryInterface` | `getUsage()` |

Refer to each interface header in `Os/Interfaces/` for the full method signatures and status enums.

---

## Step 7 — Use the Implementation on a Platform

Some guidance is available in the [Porting Guide](porting-guide.md), and a full How-To will soon follow on how to port F Prime to a new platform.

The bottom line is that to use your new OSAL implementation, you need to add the `Os_<Module>_<Suffix>` target to the `CHOOSES_IMPLEMENTATIONS` list of a `register_fprime_config` call in your platform CMake files. That one choice links the implementation and places its selection header (Step 3) at `config/<Module>Selection.hpp`. A project replaces a choice of its platform by choosing another implementation from its own configuration module. Examples are available:

- [F Prime Zephyr Support Package](https://github.com/fprime-community/fprime-zephyr/tree/main/cmake/platform)
- [F Prime VxWorks Support Package](https://github.com/fprime-community/fprime-vxworks/tree/devel/cmake/platform)

---

## Best Practices

- **Start with Stubs.** Only implement the modules you need. The [Stub backend](../../../Os/Stub) returns `NOT_SUPPORTED` for unimplemented services, allowing you to bring up an OSAL incrementally.
- **Map error codes consistently.** Create a shared error-translation module (like `Os/Posix/error.hpp`) that converts your OS's native error codes to the F´ Os `Status` enums. This keeps error handling centralized and testable.
- **Watch the handle size.** Each implementation object must fit in the fixed-size handle storage. If your native OS primitives are large, override `FW_MUTEX_HANDLE_MAX_SIZE` (or equivalent) in your platform configuration.
- **Use other implementations as references.** The `Os/Posix/` directory is the most complete OSAL implementation in F´ and serves as the canonical example for all modules. Other implementations exist (see links above, or in the resources below)
- **Test with the F´ OSAL unit tests.** The framework provides OS-agnostic unit tests under `Os/test/` that exercise the OSAL interfaces. Run these tests against your implementation to validate correctness.

---

## Additional Resources

- [OSAL Software Design Document](../../../Os/docs/sdd.md) — Architecture details and design rationale.
- [Develop an F´ Library](../develop/develop-fprime-libraries.md) — Library structure, toolchain, and platform files.
- [Porting Guide](porting-guide.md) — High-level checklist for porting F´ to a new platform.
- [Posix OSAL implementation](https://github.com/nasa/fprime/tree/devel/Os/Posix) — Reference implementation for POSIX systems.
- [fprime-zephyr](https://github.com/fprime-community/fprime-zephyr) — OSAL implementation for Zephyr RTOS.
- [fprime-vxworks](https://github.com/fprime-community/fprime-vxworks) - OSAL implementation for VxWorks RTOS.
- [fprime-freertos](https://github.com/fprime-community/fprime-freertos/tree/main/FreeRTOS/Os) - OSAL implementation for FreeRTOS.
