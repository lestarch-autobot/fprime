####
# Ref/config/CMakeLists.txt:
#
# Test deployment configuration: replaces the CdhCore subtopology's telemetry sender (Svc.TlmChan) with
# Svc.TlmPacketizer. Build with -DFPRIME_TLM_PACKETIZER_OMIT_LIST=OFF (setPacketList is given IGNORE_OMIT_LIST).
####
register_fprime_config(
    CONFIGURATION_OVERRIDES
        "${CMAKE_CURRENT_LIST_DIR}/CdhCoreTlmConfig.fpp"
    INTERFACE
)
