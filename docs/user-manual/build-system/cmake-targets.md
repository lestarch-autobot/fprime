
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

In unit-test builds (`BUILD_TESTING=ON`) every test carries the CTest label `fprime_universe`, and the tests built by
`all` also carry `fprime_project`. `ctest -L fprime_project` runs the project's tests only and `ctest -L fprime_universe`
runs every test. The label names are recorded in the CMake cache as `FPRIME_PROJECT_TEST_LABEL` and
`FPRIME_UNIVERSE_TEST_LABEL`. fprime-tools releases after 4.4.0 run by these labels: `fprime-util check` at the project
root and `fprime-util check --project` (from any directory of the project) run the `fprime_project` tests, and
`fprime-util check --all` runs the `fprime_universe` tests. The tests built by `all` are also listed by name in the
root `tests.fprime-util`, which earlier fprime-util versions use to select the same tests with a `ctest -R` expression.

A project removes a test from its project test set with `fprime_exclude_test_from_project(<test name>)`: the test
loses the `fprime_project` label and its root `tests.fprime-util` entry, and is still built and labeled
`fprime_universe`. See the [API](../../reference/api/cmake/API.md) for when to call it.

A module may exclude itself regardless of this option with the `EXCLUDE_FROM_ALL` argument of its `register_fprime_*`
call (or of `add_fprime_subdirectory`). `EXCLUDE_FROM_ALL` has its CMake meaning however it was set: the module is left
out of `all`, builds when named or depended upon, and is part of `universe`.

When F Prime itself is the top-level CMake project (building the framework), the option is forced `OFF` so that `all`
builds everything. Projects restore that behavior with `FPRIME_EXCLUDE_LIBRARIES_FROM_ALL=OFF`
(`default_cmake_options` in `settings.ini`, a `set()` before `find_package(FPrime)`/`add_subdirectory`, or `-D`).


### `dictionary_check`

The dictionary check stage ([cmake/target/dictionary_check.cmake](https://github.com/nasa/fprime/tree/devel/cmake/target/dictionary_check.cmake))
checks a deployment's topology dictionary against the configuration of the modules it uses, so that a mismatch fails
the deployment's build instead of surfacing at run time. It defines no target of its own: the checks run as part of
building the deployment.

A module that owns a check registers a checker script from its `CMakeLists.txt`:

```cmake
register_fprime_dictionary_check("${CMAKE_CURRENT_LIST_DIR}/tools/my_check.py" VARIABLES MY_CHECK_SETTING)
```

For every deployment that depends on the registering module, and for each `<Deployment>TopologyDictionary.json` of that
deployment, the stage runs the script and compiles the C++ source file it generates into the deployment:

```bash
<python> <script> <Deployment>TopologyDictionary.json -o <Deployment><CheckerName>.cpp [-D <variable>=<value>]...
```

| Item | Contract |
| --- | --- |
| `<CheckerName>` | The script's file name without extension, converted from snake_case to CamelCase (`my_check.py` gives `MyCheck`). |
| Output | A C++ source file, typically of `static_assert`s on the module's configuration constants. The message of a failed assertion is what the integrator sees, so it names the constant to change and the value needed. |
| Exit code | `0` when the output was written. Non-zero when the dictionary cannot be checked: the script prints a one-line diagnostic to standard error and writes no output, and the build fails. |
| `-D <variable>=<value>` | Passed for each CMake variable named under `VARIABLES` that is defined when the deployment is registered, with its value at that point. A deployment sets such a variable in its own `CMakeLists.txt` before `register_fprime_deployment`. The script supplies the default for a variable that is not passed. |
| Applicability | A deployment that does not depend on the registering module is not checked. |
| Ordering | A checker must be registered before the deployments it applies to are registered. Framework modules are registered before a project's deployments. |

`Svc/TlmPacketizer` registers `tools/tlm_packetizer_sizing.py`, which checks the telemetry packet sets against
`MAX_PACKETIZER_MEMBERSHIPS` and `MAX_PACKETIZER_CHANNELS`; see the
[TlmPacketizer SDD](https://github.com/nasa/fprime/blob/devel/Svc/TlmPacketizer/docs/sdd.md).


## Adding Custom Targets

See the [Customization Guide](cmake-customization.md) for a description of adding custom targets.
