# Require some implementations
register_fprime_config(
    INTERFACE
    REQUIRES_IMPLEMENTATIONS Test_Implementation Test_Override
)

# Now create some implementations. This happens to be a convenient place, but not required to be here.
register_fprime_implementation(
        Test_Implementation_Platform
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/good_implementation.cpp"
    IMPLEMENTS
        Test_Implementation
)

register_fprime_implementation(
        Test_Implementation_Override
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/bad.cpp"
    IMPLEMENTS
        Test_Implementation
)

register_fprime_implementation(
        Test_Override_Override
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/good_override.cpp"
    IMPLEMENTS
        Test_Override
)

register_fprime_implementation(
        Test_Override_Unused
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/bad.cpp"
    IMPLEMENTS
        Test_Override
)

####
# CONFIGURATION_HEADERS: implementations supplying headers placed at config/<header name> when chosen
#
# The _TEST_IMPLEMENTATION_* options each set up one of the errors detected for these implementations.
####
register_fprime_config(
        Test_Configured_Requirement
    INTERFACE
    REQUIRES_IMPLEMENTATIONS Test_Configured Test_Paired
)

# Chosen by the platform, but replaced by the later choice of Test_Configured_Second: its header is not placed
register_fprime_implementation(
        Test_Configured_First
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/bad.cpp"
    IMPLEMENTS
        Test_Configured
    CONFIGURATION_HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/first/TestConfigured.hpp"
)

set(TEST_SECOND_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/second/TestConfigured.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/second/TestShared.hpp"
)
if (_TEST_IMPLEMENTATION_MISSING_HEADER)
    list(APPEND TEST_SECOND_HEADERS "${CMAKE_CURRENT_LIST_DIR}/second/DoesNotExist.hpp")
endif()
if (_TEST_IMPLEMENTATION_NO_HEADERS)
    register_fprime_implementation(
            Test_Configured_Second
        SOURCES
            "${CMAKE_CURRENT_LIST_DIR}/good_configured.cpp"
        IMPLEMENTS
            Test_Configured
    )
else()
    register_fprime_implementation(
            Test_Configured_Second
        SOURCES
            "${CMAKE_CURRENT_LIST_DIR}/good_configured.cpp"
        IMPLEMENTS
            Test_Configured
        CONFIGURATION_HEADERS
            ${TEST_SECOND_HEADERS}
    )
endif()

# Chosen implementations of different packages may supply a header of the same name when the content is identical
set(TEST_PAIRED_HEADER "${CMAKE_CURRENT_LIST_DIR}/paired/TestShared.hpp")
if (_TEST_IMPLEMENTATION_DIFFERING_HEADER)
    set(TEST_PAIRED_HEADER "${CMAKE_CURRENT_LIST_DIR}/differing/TestShared.hpp")
endif()
register_fprime_implementation(
        Test_Paired_Paired
    IMPLEMENTS
        Test_Paired
    INTERFACE
    CONFIGURATION_HEADERS
        "${TEST_PAIRED_HEADER}"
)

# Chosen after the platform: the last choice for Test_Configured wins
register_fprime_config(
        Test_Configured_Choice
    INTERFACE
    CHOOSES_IMPLEMENTATIONS
        Test_Configured_Second
        Test_Paired_Paired
)

# A configuration module may not supply a file named as a chosen CONFIGURATION_HEADERS file
if (_TEST_IMPLEMENTATION_CONFIG_COLLISION)
    register_fprime_config(
            Test_Configured_Collision
        INTERFACE
        HEADERS
            "${CMAKE_CURRENT_LIST_DIR}/differing/TestShared.hpp"
    )
endif()

# CONFIGURATION_OVERRIDES may not replace a chosen CONFIGURATION_HEADERS file: it is no configuration module's file
if (_TEST_IMPLEMENTATION_CONFIG_OVERRIDE)
    register_fprime_config(
            Test_Configured_Override
        INTERFACE
        CONFIGURATION_OVERRIDES
            "${CMAKE_CURRENT_LIST_DIR}/differing/TestShared.hpp"
    )
endif()

# Neither a deployment nor a unit test may choose an implementation supplying CONFIGURATION_HEADERS
if (_TEST_IMPLEMENTATION_DEPLOYMENT_CHOICE)
    register_fprime_deployment(
            Test_Choosing_Deployment
        SOURCES
            "${CMAKE_CURRENT_LIST_DIR}/good_configured.cpp"
        CHOOSES_IMPLEMENTATIONS
            Test_Configured_Second
    )
endif()
if (_TEST_IMPLEMENTATION_UT_CHOICE)
    register_fprime_ut(
            Test_Choosing_Ut
        SOURCES
            "${CMAKE_CURRENT_LIST_DIR}/good_configured.cpp"
        CHOOSES_IMPLEMENTATIONS
            Test_Configured_Second
    )
endif()
