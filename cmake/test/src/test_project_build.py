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


def test_project_build_excludes_unused_included_code(PROJECT_BUILD):
    """'all' builds the deployment and its dependencies but not unrelated framework modules"""
    cmake.assert_process_success(PROJECT_BUILD)
    library_directory = PROJECT_BUILD["build"] / "lib" / TOOLCHAIN_NAME
    assert (PROJECT_BUILD["build"] / "bin" / TOOLCHAIN_NAME / "TestDeployment").exists()
    assert (
        library_directory / USED_LIBRARY_MODULE
    ).exists(), "Dependency of the deployment not built"
    assert not (
        library_directory / UNUSED_FRAMEWORK_LIBRARY
    ).exists(), "Unused framework module built by 'all'"


def test_excluded_module_builds_on_demand(PROJECT_BUILD):
    """An excluded framework module still builds when named directly"""
    cmake.assert_process_success(PROJECT_BUILD)
    return_code, _, _ = cmake.run_make(PROJECT_BUILD["build"], "Svc_Health")
    assert return_code == 0, "Excluded module failed to build on demand"
    assert (
        PROJECT_BUILD["build"] / "lib" / TOOLCHAIN_NAME / UNUSED_FRAMEWORK_LIBRARY
    ).exists()


def test_universe_target_exists(PROJECT_BUILD):
    """The 'universe' target is defined and is a no-op for already-built dependencies of the project"""
    cmake.assert_process_success(PROJECT_BUILD)
    return_code, stdout, _ = cmake.run_make(PROJECT_BUILD["build"], "help")
    assert return_code == 0
    assert any("universe" in line for line in stdout), "'universe' target not defined"


def test_option_off_builds_everything(EVERYTHING_BUILD):
    """With FPRIME_EXCLUDE_LIBRARIES_FROM_ALL=OFF, 'all' builds unrelated framework modules too"""
    cmake.assert_process_success(EVERYTHING_BUILD)
    assert (
        EVERYTHING_BUILD["build"] / "lib" / TOOLCHAIN_NAME / UNUSED_FRAMEWORK_LIBRARY
    ).exists()
