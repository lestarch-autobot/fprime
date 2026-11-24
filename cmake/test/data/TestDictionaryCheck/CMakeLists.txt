####
# 'TestDictionaryCheck' Deployment:
#
# A deployment used to test the dictionary check stage with the Svc::TlmPacketizer sizing check. Options:
#
# - TEST_MAX_PACKETIZER_MEMBERSHIPS, TEST_MAX_PACKETIZER_CHANNELS: table sizes to configure
# - TEST_OMIT_LIST: value to set FPRIME_TLM_PACKETIZER_OMIT_LIST to
# - TEST_NO_TLM_PACKETIZER: the deployment does not depend on Svc_TlmPacketizer
####

# Base settings
cmake_minimum_required(VERSION 3.18)
cmake_policy(SET CMP0048 NEW)
project(TestDictionaryCheck VERSION 1.0.0 LANGUAGES C CXX)

include("${FPRIME_FRAMEWORK_PATH}/cmake/FPrime.cmake")
# NOTE: register custom targets between these two lines
include("${FPRIME_FRAMEWORK_PATH}/cmake/FPrime-Code.cmake")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/config")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Top")

set(TEST_DEPENDS TestDictionaryCheck_Top)
if (NOT TEST_NO_TLM_PACKETIZER)
    list(APPEND TEST_DEPENDS Svc_TlmPacketizer)
endif()
if (DEFINED TEST_OMIT_LIST)
    set(FPRIME_TLM_PACKETIZER_OMIT_LIST "${TEST_OMIT_LIST}")
endif()

register_fprime_deployment(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/Main.cpp"
  DEPENDS
    ${TEST_DEPENDS}
)
