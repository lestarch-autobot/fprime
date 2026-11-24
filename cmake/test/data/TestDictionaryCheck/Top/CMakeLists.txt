####
# Topology of the 'TestDictionaryCheck' deployment: supplies the topology dictionary.
####
register_fprime_module(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/topology.fpp"
  DEPENDS
    Svc_PassiveRateGroup
    Svc_TlmPacketizer_config_TlmPacketizerConfig
)
