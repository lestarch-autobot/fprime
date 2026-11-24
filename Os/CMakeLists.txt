####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding files
# MOD_DEPS: (optional) module dependencies
#
####

####
# WARNING: not for use outside of fprime/Os
#
# Adds a named os module (e.g. common code for an OSAL implementation). Helper to allow multiple commons per file.
#
# NAMES: list of named files to add to this module.  The first will be treated as the name of the module.
#        i.e. File;Directory;FileSystem will contain the file, directory, and filesystem files in a module called File.
# ARGN: extra MOD_DEPS to add
####
function(add_named_os_module NAMES)
    # Get first element of list
    list(GET NAMES 0 FIRST_ITEM)
    set(OS_MODULE_NAME "Os_${FIRST_ITEM}")
    # Clear variables
    set(SOURCE_INPUTS)
    set(HEADER_INPUTS)
    # Add files for each name supplied
    foreach (NAME IN LISTS NAMES)
        list(APPEND SOURCE_INPUTS "${CMAKE_CURRENT_LIST_DIR}/${NAME}.cpp")
        list(APPEND HEADER_INPUTS "${CMAKE_CURRENT_LIST_DIR}/${NAME}.hpp")
    endforeach ()
    # Set up module
    register_fprime_module(
          "${OS_MODULE_NAME}"
        REQUIRES_IMPLEMENTATIONS
          "Os_${FIRST_ITEM}"
        SOURCES
          ${SOURCE_INPUTS}
        HEADERS
          ${HEADER_INPUTS}
        DEPENDS
          Fw_Time
          Fw_Types ${ARGN}
    )
    fprime_target_dependencies(Os PUBLIC "${OS_MODULE_NAME}")
endfunction()

####
# WARNING: not for use outside of fprime/Os
#
# Adds a named os implementation supplied by fprime core. Helper to allow multiple implementations per file.
#
# NAMES: list of named files to add to this module.  The first will be treated as the name of the module.
#        i.e. File;Directory;FileSystem will contain the file, directory, and filesystem files in a module called File.
# SUFFIX: suffix to implementation (e.g. Posix)
# ARGN: extra MOD_DEPS to add
####
function(add_fprime_supplied_os_module NAMES SUFFIX)
    # Get first element of list
    list(GET NAMES 0 FIRST_ITEM)

    # Ensure expected files with expected filenames exist
    foreach (NAME IN LISTS NAMES)
        # TODO: the service .cpp files only include their header (all definitions live in the .hpp). Remove them
        # and this requirement when the Os CMake/config selection is reworked.
        if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${NAME}.cpp")
            message(FATAL_ERROR "${CMAKE_CURRENT_LIST_DIR}/${NAME}.cpp does not exist.")
        endif()
        if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${NAME}.hpp")
            message(FATAL_ERROR "${CMAKE_CURRENT_LIST_DIR}/${NAME}.hpp does not exist.")
        endif()
    endforeach()

    # Setup variables
    set(OS_MODULE_NAME "Os_${FIRST_ITEM}_${SUFFIX}")
    # Clear variables
    set(SOURCE_INPUTS)
    set(HEADER_INPUTS)
    # Add files for each name supplied
    foreach (NAME IN LISTS NAMES)
        list(APPEND SOURCE_INPUTS "${CMAKE_CURRENT_LIST_DIR}/${NAME}.cpp")
        list(APPEND HEADER_INPUTS "${CMAKE_CURRENT_LIST_DIR}/${NAME}.hpp")
    endforeach ()
    # Set up module
    register_fprime_module(
        "${OS_MODULE_NAME}_Implementation"
      SOURCES
        ${SOURCE_INPUTS}
      HEADERS
        ${HEADER_INPUTS}
      DEPENDS
        Fw_Time
        Fw_Types
        "Os_${FIRST_ITEM}"
        ${ARGN}
    )
    # The implementation is selected in config/OsSelection.hpp
    register_fprime_implementation(
         "${OS_MODULE_NAME}"
      IMPLEMENTS
        "Os_${FIRST_ITEM}"
      INTERFACE
    )
    # Os_<Name> instantiates the common class on the implementation: link the whole implementation
    # as nothing earlier in the link order references it
    target_link_libraries("${OS_MODULE_NAME}" INTERFACE
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,${OS_MODULE_NAME}_Implementation>")
endfunction()

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Models")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Stub")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Posix")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Generic")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Linux")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Darwin")

register_fprime_module(
      Os
    AUTOCODER_INPUTS
      "${CMAKE_CURRENT_LIST_DIR}/Types.fpp"
    SOURCES
      "${CMAKE_CURRENT_LIST_DIR}/ValidateFileCommon.cpp"
      "${CMAKE_CURRENT_LIST_DIR}/ValidatedFile.cpp"
      "${CMAKE_CURRENT_LIST_DIR}/IntervalTimer.cpp"
      "${CMAKE_CURRENT_LIST_DIR}/Os.cpp"
      "${CMAKE_CURRENT_LIST_DIR}/FilePathUtils.cpp"
      "${CMAKE_CURRENT_LIST_DIR}/SandboxedFile.cpp"
    HEADERS
      "${CMAKE_CURRENT_LIST_DIR}/ValidatedFile.hpp"
      "${CMAKE_CURRENT_LIST_DIR}/Os.hpp"
      "${CMAKE_CURRENT_LIST_DIR}/FilePathUtils.hpp"
      "${CMAKE_CURRENT_LIST_DIR}/SandboxedFile.hpp"
    DEPENDS
      Fw_Time
      Fw_Types
)

add_named_os_module(Console Fw_Logger)
add_named_os_module("File;FileSystem;Directory" Utils_Hash Fw_DataStructures)
# Unit test builds select Os::Ut::File::UtFile, which wraps the non-test selection (see config/OsSelection.hpp)
if (BUILD_TESTING)
    target_sources(Os_File PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Ut/File.cpp")
endif()
add_named_os_module(Task ${CMAKE_THREAD_LIBS_INIT})
# Os/Task.hpp includes the implementation header selected in config/OsSelection.hpp. The Posix
# implementation header needs the constants autocoded by Os_Posix_Models, thus users of the Os
# headers must build after Os_Posix_Models.
if (TARGET Os_Posix_Models)
    fprime_target_dependencies(Os_Models PUBLIC Os_Posix_Models)
    fprime_target_dependencies(Os_Task PUBLIC Os_Posix_Models)
endif()
# ConditionVariable (Condition.hpp) is bundled into Os_Mutex: it operates on the selected Os::Mutex
add_named_os_module("Mutex;Condition")
add_named_os_module(CountingSemaphore)
add_named_os_module(Queue)
add_named_os_module(Cpu)
add_named_os_module(Memory)
add_named_os_module(RawTime Fw_Buffer)
# Unit test builds select Os::Ut::RawTime::UtRawTime, which wraps the non-test selection (see config/OsSelection.hpp)
if (BUILD_TESTING)
    target_sources(Os_RawTime PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Ut/RawTime.cpp")
endif()

### UTS ### Note: 3 separate UTs registered here.
register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/OsTestMain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/IntervalTimerTest.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/OsValidateFileTest.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/OsMutexBasicLockableTest.cpp"
)

register_fprime_ut(
    Os_FilePathUtils_Test
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/FilePathUtilsTest.cpp"
  DEPENDS
    Os
)

register_fprime_ut(
    Os_SandboxedFile_Test
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/SandboxedFileTest.cpp"
  DEPENDS
    Os
)

if (BUILD_TESTING)
    register_fprime_module(Os_Test_File_SyntheticFileSystem
      SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/file/SyntheticFileSystem.cpp"
      HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/file/SyntheticFileSystem.hpp"
      DEPENDS
        Fw_Types
    )
endif()
