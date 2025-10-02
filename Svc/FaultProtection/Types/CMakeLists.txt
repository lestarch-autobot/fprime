# This package while logically part of Types is a dependency of FaultConfig and thus must build separately.
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/BaseTypes")

# This module depends **on** FaultConfig and thus cannot build as a single unit with the first module defined in this
# folder. These types expand on FaultConfig.
register_fprime_library(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/Types.fpp"
    DEPENDS
        Fw_Types
        Svc_FaultProtection_Types_BaseTypes # Implied through fault config, but explicit is better
        Svc_FaultProtection_FaultConfig
)
