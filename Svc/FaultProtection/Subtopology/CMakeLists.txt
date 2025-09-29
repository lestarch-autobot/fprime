add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/FaultProtectionConfig")

register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/Subtopology.fpp"
    DEPENDS
        Svc_FaultProtection_Subtopology_FaultProtectionConfig
    EXCLUDE_FROM_ALL
)