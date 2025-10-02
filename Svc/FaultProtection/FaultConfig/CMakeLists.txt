register_fprime_config(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/FaultConfig.fpp"
    DEPENDS
        Svc_FaultProtection_Types_BaseTypes # Brings in type definitions used within config
)
