####
# test_basic.py:
#
# Basic CMake tests.
#
####
import pytest

from . import cmake
from . import settings


def implementation_build(fixture_name, option=None, make_targets=None, unit_tests=False):
    """Build fixture of the implementations deployment, optionally with one of its _TEST_IMPLEMENTATION_* options"""
    arguments = {
        "FPRIME_FRAMEWORK_PATH": settings.FRAMEWORK_PATH,
        "FPRIME_PROJECT_ROOT": settings.DATA_DIR / "test-implementations",
        "FPRIME_LIBRARY_LOCATIONS": ";".join(
            [
                str(settings.DATA_DIR / "test-implementations" / "test-platforms"),
            ]
        ),
    }
    if option is not None:
        arguments[option] = "ON"
    if unit_tests:
        arguments["BUILD_TESTING"] = "ON"
    return cmake.get_build(
        fixture_name,
        settings.DATA_DIR / "test-implementations" / "Deployment",
        arguments,
        make_targets=[] if make_targets is None else make_targets,
    )


def assert_generation_error(build, expected):
    """Assert that CMake generation failed, and that the failure is the expected error"""
    return_code, _, stderr = build["cmake"]
    assert return_code != 0, "CMake generation was expected to fail"
    # CMake wraps error text, so compare with whitespace collapsed
    output = " ".join("".join(stderr).split())
    assert expected in output, f"Expected '{expected}' in:\n{''.join(stderr)}"


_ = implementation_build("IMPLEMENTATION_TEST", make_targets=["Deployment"])
_target_choice = implementation_build(
    "IMPLEMENTATION_TARGET_CHOICE_BUILD", "_TEST_IMPLEMENTATION_TARGET_CHOICE"
)
_deployment_choice = implementation_build(
    "IMPLEMENTATION_DEPLOYMENT_CHOICE_BUILD", "_TEST_IMPLEMENTATION_DEPLOYMENT_CHOICE"
)
_ut_choice = implementation_build(
    "IMPLEMENTATION_UT_CHOICE_BUILD", "_TEST_IMPLEMENTATION_UT_CHOICE", unit_tests=True
)
_config_override = implementation_build(
    "IMPLEMENTATION_CONFIG_OVERRIDE_BUILD", "_TEST_IMPLEMENTATION_CONFIG_OVERRIDE"
)
_differing_header = implementation_build(
    "IMPLEMENTATION_DIFFERING_HEADER_BUILD", "_TEST_IMPLEMENTATION_DIFFERING_HEADER"
)
_missing_header = implementation_build(
    "IMPLEMENTATION_MISSING_HEADER_BUILD", "_TEST_IMPLEMENTATION_MISSING_HEADER"
)
_no_headers = implementation_build(
    "IMPLEMENTATION_NO_HEADERS_BUILD", "_TEST_IMPLEMENTATION_NO_HEADERS"
)
_config_collision = implementation_build(
    "IMPLEMENTATION_CONFIG_COLLISION_BUILD", "_TEST_IMPLEMENTATION_CONFIG_COLLISION"
)


def test_platform_implementation(IMPLEMENTATION_TEST):
    """Check the platform-specified implementation was produced"""
    cmake.assert_process_success(IMPLEMENTATION_TEST, warnings_ok=True)


def test_override_implementation(IMPLEMENTATION_TEST):
    """Check the override-specified implementation was produced"""
    cmake.assert_process_success(IMPLEMENTATION_TEST, warnings_ok=True)


def test_non_built_implementation(IMPLEMENTATION_TEST):
    """Check the override target that wasn't use was not built along with the override platform target"""
    cmake.assert_process_success(IMPLEMENTATION_TEST, warnings_ok=True)


def test_configuration_headers_placed(IMPLEMENTATION_TEST):
    """Check exactly the CONFIGURATION_HEADERS of the globally chosen implementations were placed in config/

    Test_Configured_First is chosen by the platform and Test_Configured_Second by a later configuration module: the
    last choice wins. Main.cpp asserts the content of the headers at compile time.
    """
    cmake.assert_process_success(IMPLEMENTATION_TEST, warnings_ok=True)
    config = IMPLEMENTATION_TEST["build"] / "fprime-implementation-config" / "config"
    # The framework's own implementations place headers as well
    assert sorted(path.name for path in config.glob("Test*.hpp")) == [
        "TestConfigured.hpp",
        "TestShared.hpp",
    ]
    source = (
        IMPLEMENTATION_TEST["source"] / "TestModule" / "second" / "TestConfigured.hpp"
    )
    assert (config / "TestConfigured.hpp").read_text() == source.read_text()


def test_configuration_headers_target_choice(IMPLEMENTATION_TARGET_CHOICE_BUILD):
    """Check an executable may not choose an implementation supplying CONFIGURATION_HEADERS"""
    assert_generation_error(
        IMPLEMENTATION_TARGET_CHOICE_BUILD,
        "Deployment chooses Test_Configured_Second, which supplies CONFIGURATION_HEADERS",
    )


def test_configuration_headers_differing(IMPLEMENTATION_DIFFERING_HEADER_BUILD):
    """Check chosen implementations may not supply differing headers of the same name"""
    assert_generation_error(
        IMPLEMENTATION_DIFFERING_HEADER_BUILD,
        "supply differing CONFIGURATION_HEADERS named TestShared.hpp",
    )


def test_configuration_headers_missing(IMPLEMENTATION_MISSING_HEADER_BUILD):
    """Check CONFIGURATION_HEADERS must exist"""
    assert_generation_error(
        IMPLEMENTATION_MISSING_HEADER_BUILD,
        "Test_Configured_Second supplies CONFIGURATION_HEADERS file that does not exist",
    )


def test_configuration_headers_not_supplied(IMPLEMENTATION_NO_HEADERS_BUILD):
    """Check a chosen implementation must supply CONFIGURATION_HEADERS when its alternatives do"""
    assert_generation_error(
        IMPLEMENTATION_NO_HEADERS_BUILD,
        "Test_Configured_Second is chosen for Test_Configured but supplies no CONFIGURATION_HEADERS",
    )


def test_configuration_headers_config_collision(IMPLEMENTATION_CONFIG_COLLISION_BUILD):
    """Check a configuration module may not supply a file named as a chosen CONFIGURATION_HEADERS file"""
    assert_generation_error(
        IMPLEMENTATION_CONFIG_COLLISION_BUILD,
        "configuration module Test_Configured_Collision supplies TestShared.hpp",
    )


def test_configuration_headers_deployment_choice(IMPLEMENTATION_DEPLOYMENT_CHOICE_BUILD):
    """Check a deployment may not choose an implementation supplying CONFIGURATION_HEADERS"""
    assert_generation_error(
        IMPLEMENTATION_DEPLOYMENT_CHOICE_BUILD,
        "Test_Choosing_Deployment chooses Test_Configured_Second, which supplies CONFIGURATION_HEADERS",
    )


def test_configuration_headers_ut_choice(IMPLEMENTATION_UT_CHOICE_BUILD):
    """Check a unit test may not choose an implementation supplying CONFIGURATION_HEADERS"""
    assert_generation_error(
        IMPLEMENTATION_UT_CHOICE_BUILD,
        "Test_Choosing_Ut chooses Test_Configured_Second, which supplies CONFIGURATION_HEADERS",
    )


def test_configuration_headers_config_override(IMPLEMENTATION_CONFIG_OVERRIDE_BUILD):
    """Check CONFIGURATION_OVERRIDES may not replace a chosen CONFIGURATION_HEADERS file"""
    assert_generation_error(
        IMPLEMENTATION_CONFIG_OVERRIDE_BUILD,
        "TestShared.hpp is CONFIGURATION_OVERRIDE but overrides nonexistent file",
    )
