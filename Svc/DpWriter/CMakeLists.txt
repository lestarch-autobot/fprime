register_fprime_module(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/DpWriter.fpp"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/DpWriter.cpp"
  DEPENDS
    Fw_Dp
)

# UT_AUTO_HELPERS is not used: the tester hand-writes connectPorts() and
# initComponents() to leave the last processing port unconnected
register_fprime_ut(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/DpWriter.fpp"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/AbstractState.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/DpWriterTestMain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/DpWriterTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/Rules/BufferSendIn.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/Rules/CLEAR_EVENT_THROTTLE.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/Rules/FileOpenStatus.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/Rules/FileWriteStatus.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/Rules/SchedIn.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/Rules/SET_PROC_OVERRIDE.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/Rules/Testers.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/Scenarios/Random.cpp"
  DEPENDS
    STest
  CHOOSES_IMPLEMENTATIONS
    Os_File_Test_Stub
)
