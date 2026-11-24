restrict_platforms(Posix)

register_fprime_module(
        Svc_FileWorker_test_FileTester
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/FileTester.cpp"
    HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/FileTester.hpp"
    DEPENDS
        Fw_Types
        Os
)
