####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding files
# MOD_DEPS: (optional) module dependencies
#
# Note: using PROJECT_NAME as EXECUTABLE_NAME
####
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/config/TlmPacketizerConfig")

register_fprime_module(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/TlmPacketizer.fpp"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/TlmPacketizer.cpp"
  DEPENDS
    Svc_TlmPacketizer_config_TlmPacketizerConfig
)

# Checks each dependent deployment's packet sets against the configured table sizes at build time. A deployment
# that passes IGNORE_OMIT_LIST to setPacketList sets FPRIME_TLM_PACKETIZER_OMIT_LIST to OFF.
register_fprime_dictionary_check(
    "${CMAKE_CURRENT_LIST_DIR}/tools/tlm_packetizer_sizing.py"
  VARIABLES
    FPRIME_TLM_PACKETIZER_OMIT_LIST
)

### UTs ###
register_fprime_ut(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/TlmPacketizer.fpp"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/main.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/TlmPacketizerTester.cpp"
)
set (UT_TARGET_NAME "${FPRIME_CURRENT_MODULE}_ut_exe")
if (TARGET "${UT_TARGET_NAME}")
    target_compile_options("${UT_TARGET_NAME}" PRIVATE -Wno-conversion)
endif()
