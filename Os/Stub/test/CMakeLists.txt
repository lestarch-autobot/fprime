####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding files
# MOD_DEPS: (optional) module dependencies
#
####
if (NOT BUILD_TESTING)
    return()
endif ()
add_custom_target("${FPRIME_CURRENT_MODULE}")

add_compile_options(
    -Wno-conversion
)

# Test support for Os::File: an interceptor answering every Os::File call from injectable static data
register_fprime_module(Os_Stub_Test_File
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/File.cpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/File.hpp"
  DEPENDS
    Fw_Types
    Os
)

#### File Stub Testing ####
register_fprime_ut(
    StubFileTest
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ut/StubFileTests.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../test/ut/file/CommonTests.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../test/ut/file/FileRules.cpp"
  DEPENDS
    Os_Stub_Test_File
    Fw_Types
    Os
    Os_Test_File_SyntheticFileSystem
    STest
)

#### Console Stub Testing ####
register_fprime_ut(
    StubConsoleTest
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ut/StubConsoleTests.cpp"
  DEPENDS
    Os
    Fw_Types
    STest
)


#### Cpu Stub Testing ####
register_fprime_ut(
    StubCpuTest
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ut/StubCpuTests.cpp"
  DEPENDS
    Os
    Fw_Types
    STest
)

#### Memory Stub Testing ####
register_fprime_ut(
    StubMemoryTest
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ut/StubMemoryTests.cpp"
  DEPENDS
    Os
    Fw_Types
    STest
)

#### Queue Stub Testing ####
register_fprime_ut(
    StubQueueTest
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ut/StubQueueTests.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../test/ut/queue/QueueRules.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../test/ut/queue/CommonTests.cpp"
  DEPENDS
    Os
    Fw_Types
    STest
)
if (TARGET StubQueueTest)
    target_include_directories(StubQueueTest PRIVATE "${CMAKE_CURRENT_LIST_DIR}/ut")
endif ()

#### Task Stub Testing ####
register_fprime_ut(
    StubTaskTest
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ut/StubTaskTests.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../test/ut/task/CommonTests.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../../test/ut/task/TaskRules.cpp"
  DEPENDS
    Os
    Fw_Types
    STest
)

#### Mutex Interface Testing ####
register_fprime_ut(
    StubMutexTest
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ut/StubMutexTests.cpp"
  DEPENDS
    Os
    Fw_Types
    STest
)

#### FileSystem Stub Testing ####
register_fprime_ut(
    StubFileSystemTest
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ut/StubFileSystemTests.cpp"
  DEPENDS
    Os
    Fw_Types
    Fw_Time
    STest
)

#### Directory Stub Testing ####
register_fprime_ut(
    StubDirectoryTest
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ut/StubDirectoryTests.cpp"
  DEPENDS
    Os
    Fw_Types
    Fw_Time
    STest
)

## Condition variable tests
register_fprime_ut(
    StubConditionVariableTest
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ut/StubConditionTests.cpp"
  DEPENDS
    Os
    Fw_Types
    Fw_Time
    STest
)

#### RawTime Stub Testing ####
register_fprime_ut(
    StubRawTimeTest
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ut/StubRawTimeTests.cpp"
  DEPENDS
    Os
    Fw_Types
    Fw_Time
    STest
)