
# Targets

The build system supplies the ability to register extra targets that allow the user to register new
functionality to the build system. For example, the user may wish to create a build target that
counts the total number of files in the system.  This would be the place to register such a target
and have it available to the build system.

Targets are applied both at the global scope and per-module scope. Thus each target can provide a set of build targets (one per registered module) and a global build target. For example, a counting target might provide a `count` global target to count all files, and a `<MODULE>_count` to count the files of a given module.

## Built-In Targets

The CMake system supplies several targets that are useful for all projects and thus are included
as part of the CMake system. These targets are defined in [cmake/target](https://github.com/nasa/fprime/tree/devel/cmake/target).

### `all` and `universe`

Two targets come from CMake and the module registration itself rather than from `cmake/target`:

CMake's `all` target builds the project: the modules, deployments, and unit tests registered by the project, plus the
framework and library modules they depend upon. Framework and library modules the project does not use are registered
`EXCLUDE_FROM_ALL` (option `FPRIME_EXCLUDE_LIBRARIES_FROM_ALL`, default `ON`) and are built only when named directly or
depended upon. The `universe` target builds every registered module, excluded or not:

```bash
cmake --build . --target all        # the project and what it depends upon (fprime-util build at the project root)
cmake --build . --target universe   # everything the build system knows about
```

In unit-test builds (`BUILD_TESTING=ON`) the tests of excluded modules carry the CTest label `fprime_universe`, so
`ctest -LE fprime_universe` runs the project's tests only, and `fprime-util check` at the project root runs exactly
those tests. `fprime-util check --all` runs CTest without a filter, i.e. every test. fprime-tools releases after 4.4.0
also offer `fprime-util build --project` and `check --project` to do the same from any directory of the project.

A module may exclude itself regardless of this option with the `EXCLUDE_FROM_ALL` argument of its `register_fprime_*`
call (or of `add_fprime_subdirectory`); such a module is also left out of `universe` and builds only when named or
depended upon, and a unit test so excluded is skipped by deployment unit-test roll-ups.

When F Prime itself is the top-level CMake project (building the framework), the option is forced `OFF` so that `all`
builds everything. Projects restore that behavior with `FPRIME_EXCLUDE_LIBRARIES_FROM_ALL=OFF`
(`default_cmake_options` in `settings.ini`, a `set()` before `find_package(FPrime)`/`add_subdirectory`, or `-D`).


## Adding Custom Targets

See the [Customization Guide](cmake-customization.md) for a description of adding custom targets.
