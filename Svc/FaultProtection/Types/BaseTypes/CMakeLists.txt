# FaultConfig depends on these type definitions. Thus these are pulled into a separate build module from Types to ensure
# that the types are available to FaultConfig. This was excluded from FaultConfig build modules as these types are not
# user-mutable and thus should not be added to a config call.
register_fprime_library(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/BaseTypes.fpp"
    DEPENDS
        Fw_Types
)
