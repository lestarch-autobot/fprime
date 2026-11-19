####
# F Prime CMakeLists.txt:
#
# SOURCES: list of source files (to be compiled)
# AUTOCODER_INPUTS: list of files to be passed to the autocoders
# DEPENDS: list of libraries that this module depends on
####
register_fprime_module(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/SeqScheduler.cpp"
  DEPENDS
    Fw_DataStructures
    Os
    Utils_Hash
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/SeqScheduler.fpp"
)

register_fprime_ut(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/SeqScheduler.fpp"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/SeqSchedulerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/SeqSchedulerTestMain.cpp"
  DEPENDS
    STest
  UT_AUTO_HELPERS
)
