####
# F Prime CMakeLists.txt:
#
# SOURCES: list of source files (to be compiled)
# AUTOCODER_INPUTS: list of files to be passed to the autocoders
# DEPENDS: list of libraries that this module depends on
####
register_fprime_library(
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/TimeBaseSetter.cpp"
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/TimeBaseSetter.fpp"
)
