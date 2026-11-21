####
# Project configuration: overrides of the framework defaults for the test deployments
####
register_fprime_config(
        TestDeploymentsConfig
    INTERFACE
    CONFIGURATION_OVERRIDES
        "${CMAKE_CURRENT_LIST_DIR}/CommandDispatcherImplCfg.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/CdhCoreConfig.fpp"
)
