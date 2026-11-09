####
# test_project_build.py:
#
# Tests for FPRIME_EXCLUDE_LIBRARIES_FROM_ALL: a project build ("all") builds the project's modules and only the
# framework and library modules they depend upon, excluded modules still build on demand, and the option can be turned
# off to restore the historical "everything is in all" behavior.
#
####
import platform

from . import cmake
from . import settings

TOOLCHAIN_NAME = "generic-native"

# A framework module that TestDeployment does not depend upon
UNUSED_FRAMEWORK_LIBRARY = "libSvc_Health.a"
# A library module that TestDeployment depends upon (declared in test-fprime-library2)
USED_LIBRARY_MODULE = "libTestLibrary2_TestComponent.a"


def _cmake_arguments(**extra):
    """CMake arguments for the TestDeployment project with both test libraries"""
    arguments = {
        "FPRIME_FRAMEWORK_PATH": settings.FRAMEWORK_PATH,
        "FPRIME_PROJECT_ROOT": settings.DATA_DIR,
        "FPRIME_LIBRARY_LOCATIONS": ";".join(
            [
                str(settings.DATA_DIR / "test-fprime-library"),
                str(settings.DATA_DIR / "test-fprime-library2"),
            ]
        ),
        "CMAKE_TOOLCHAIN_FILE": str(
            settings.DATA_DIR
            / "test-fprime-library"
            / "cmake"
            / "toolchain"
            / f"{TOOLCHAIN_NAME}.cmake"
        ),
        "FPRIME_PLATFORM": platform.system(),
    }
    arguments.update(extra)
    return arguments


PROJECT_BUILD_RESULT = cmake.get_build(
    "PROJECT_BUILD", settings.DATA_DIR / "TestDeployment", _cmake_arguments(), ["all"]
)

EVERYTHING_BUILD_RESULT = cmake.get_build(
    "EVERYTHING_BUILD",
    settings.DATA_DIR / "TestDeployment",
    _cmake_arguments(FPRIME_EXCLUDE_LIBRARIES_FROM_ALL="OFF"),
    ["all"],
)

# Configure-only: unit-test build of the reference project (F Prime added via add_subdirectory) with framework unit
# tests enabled, to inspect CTest labels and the root test metadata
PROJECT_UT_CONFIGURE_RESULT = cmake.get_build(
    "PROJECT_UT_CONFIGURE",
    settings.REF_APP_PATH,
    {"BUILD_TESTING": "ON", "FPRIME_ENABLE_FRAMEWORK_UTS": "ON"},
    [],
)

# Configure-only: the framework as the top-level project forces the option OFF
FRAMEWORK_UT_CONFIGURE_RESULT = cmake.get_build(
    "FRAMEWORK_UT_CONFIGURE",
    settings.FRAMEWORK_PATH,
    {"BUILD_TESTING": "ON"},
    [],
)


def test_project_build_excludes_unused_included_code(PROJECT_BUILD):
    """'all' builds the deployment and its dependencies but not unrelated framework modules, which build on demand"""
    cmake.assert_process_success(PROJECT_BUILD)
    library_directory = PROJECT_BUILD["build"] / "lib" / TOOLCHAIN_NAME
    assert (PROJECT_BUILD["build"] / "bin" / TOOLCHAIN_NAME / "TestDeployment").exists()
    assert (
        library_directory / USED_LIBRARY_MODULE
    ).exists(), "Dependency of the deployment not built"
    assert not (
        library_directory / UNUSED_FRAMEWORK_LIBRARY
    ).exists(), "Unused framework module built by 'all'"
    # The excluded module still builds when named directly (same fixture, so the absence check above runs first)
    return_code, _, _ = cmake.run_make(PROJECT_BUILD["build"], "Svc_Health")
    assert return_code == 0, "Excluded module failed to build on demand"
    assert (library_directory / UNUSED_FRAMEWORK_LIBRARY).exists()


def test_universe_target_covers_excluded_modules(PROJECT_BUILD):
    """The 'universe' target depends on the excluded framework modules (dry run, no full framework build)"""
    cmake.assert_process_success(PROJECT_BUILD)
    return_code, stdout, _ = cmake.subprocess_helper(
        [cmake.MAKE_CALL, "-n", "universe"], PROJECT_BUILD["build"]
    )
    assert return_code == 0, "'universe' target not defined"
    assert any(
        "Svc_FileManager" in line for line in stdout
    ), "'universe' does not build excluded framework modules"


def test_option_off_builds_everything(EVERYTHING_BUILD):
    """With FPRIME_EXCLUDE_LIBRARIES_FROM_ALL=OFF, 'all' builds unrelated framework modules too"""
    cmake.assert_process_success(EVERYTHING_BUILD)
    assert (
        EVERYTHING_BUILD["build"] / "lib" / TOOLCHAIN_NAME / UNUSED_FRAMEWORK_LIBRARY
    ).exists()


def test_excluded_unit_tests_labeled(PROJECT_UT_CONFIGURE):
    """Excluded framework tests carry the fprime_universe label and the root test metadata is not the catch-all"""
    cmake.assert_process_success(PROJECT_UT_CONFIGURE, errors_ok=True)
    return_code, labeled, _ = cmake.subprocess_helper(
        ["ctest", "-N", "-L", "fprime_universe"], PROJECT_UT_CONFIGURE["build"]
    )
    assert return_code == 0
    assert any(
        "Fw_Types_ut_exe" in line for line in labeled
    ), "Framework test not labeled fprime_universe"
    root_tests = (
        (PROJECT_UT_CONFIGURE["build"] / "tests.fprime-util").read_text().split()
    )
    assert ".*" not in root_tests, "Root test metadata still lists every test"
    assert (
        "Fw_Types_ut_exe" not in root_tests
    ), "Framework test listed as a project test"


def test_framework_top_level_forces_option_off(FRAMEWORK_UT_CONFIGURE):
    """When F Prime is the top-level project the option is forced OFF: no labels, catch-all root test metadata"""
    cmake.assert_process_success(FRAMEWORK_UT_CONFIGURE, errors_ok=True)
    _, stdout, _ = FRAMEWORK_UT_CONFIGURE["cmake"]
    assert any(
        "FPRIME_EXCLUDE_LIBRARIES_FROM_ALL forced OFF" in line for line in stdout
    )
    return_code, labeled, _ = cmake.subprocess_helper(
        ["ctest", "-N", "-L", "fprime_universe"], FRAMEWORK_UT_CONFIGURE["build"]
    )
    assert return_code == 0
    assert any(
        "Total Tests: 0" in line for line in labeled
    ), "Framework build labeled its own tests"
    root_tests = (
        (FRAMEWORK_UT_CONFIGURE["build"] / "tests.fprime-util").read_text().split()
    )
    assert root_tests == [".*"]
