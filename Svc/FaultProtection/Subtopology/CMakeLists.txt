add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/FaultProtectionConfig")

register_fprime_module(
    EXCLUDE_FROM_ALL
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/Subtopology.fpp"
    HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/SubtopologyTopologyDefs.hpp"
    INTERFACE
    DEPENDS
        Svc_FaultProtection_Subtopology_FaultProtectionConfig
)