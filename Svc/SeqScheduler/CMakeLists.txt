####
# Svc/SeqScheduler: table-driven sequence scheduler
####
register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/SeqScheduler.fpp"
    DEPENDS
        Fw_Time
)
