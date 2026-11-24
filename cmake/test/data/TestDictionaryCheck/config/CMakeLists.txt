####
# Overrides TlmPacketizerCfg.hpp with a copy of the framework's file in which the table sizes are replaced by the
# values of TEST_MAX_PACKETIZER_MEMBERSHIPS and TEST_MAX_PACKETIZER_CHANNELS, when those are set.
####
set(TEST_CFG_SOURCE "${FPRIME_FRAMEWORK_PATH}/Svc/TlmPacketizer/config/TlmPacketizerConfig/TlmPacketizerCfg.hpp")
set(TEST_CFG_OVERRIDE "${CMAKE_CURRENT_BINARY_DIR}/override/TlmPacketizerCfg.hpp")
file(READ "${TEST_CFG_SOURCE}" TEST_CFG_CONTENT)
foreach(TEST_CONSTANT IN ITEMS MAX_PACKETIZER_MEMBERSHIPS MAX_PACKETIZER_CHANNELS)
    if (DEFINED TEST_${TEST_CONSTANT})
        string(REGEX REPLACE " ${TEST_CONSTANT} = [^;]+;" " ${TEST_CONSTANT} = ${TEST_${TEST_CONSTANT}};"
            TEST_CFG_REPLACED "${TEST_CFG_CONTENT}")
        if (TEST_CFG_REPLACED STREQUAL TEST_CFG_CONTENT)
            message(FATAL_ERROR "${TEST_CONSTANT} not found in ${TEST_CFG_SOURCE}")
        endif()
        set(TEST_CFG_CONTENT "${TEST_CFG_REPLACED}")
    endif()
endforeach()
file(WRITE "${TEST_CFG_OVERRIDE}" "${TEST_CFG_CONTENT}")

register_fprime_config(
        TestDictionaryCheckConfig
    CONFIGURATION_OVERRIDES
        "${TEST_CFG_OVERRIDE}"
    INTERFACE
)
