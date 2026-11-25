# CMake Implementations

Certain parts of F´ have different implementations that a project may choose from.  The canonical example of this is a file system implementation. Projects may wish to use the stubbed implementation of the file system, the platform supplied standard implementation, or something project-specific like a flash chip or SD card.

These packages must be fully specified. Thus, every platform must specify a specific implementation for every package. These specific implementations may be overridden for a specific executable deployment or unit test, unless the implementation supplies [configuration headers](#implementations-with-configuration-headers).

## Requiring an Implementation

Modules that require an implementation should use the directive `REQUIRES_IMPLEMENTATIONS` in the `register_fprime_module` call.

> [!NOTE]
> `REQUIRES_IMPLEMENTATIONS` is only needed for modules that have a direct dependency on the implementation, not on their dependents.

## Choosing an Implementation

Platform developers *must* specify an implementation of every package used in the system. Failing to do so means that a given functionality is undefined and impossible to link.  Stub implementations are provided in the case that a platform does not support a given package's functionality.

Choosing an implementation is done with the `CHOOSES_IMPLEMENTATIONS` directive available to `register_fprime_config`. Platform developers should choose implementations as part of the platform definition (see [Configuration Modules](./configuration.md#platform-packages)). A project may also choose implementations from its own configuration module; the choice registered last wins, except for an implementation an earlier configuration module already listed (choices are de-duplicated in registration order; see [Overriding Configuration in a Project](./configuration.md#overriding-configuration-in-a-project)).

https://github.com/nasa/fprime/blob/dfaf496263bdfff04461179eb99fb3f906a10009/cmake/platform/Linux.cmake#L15-L17

## Overriding an Implementation Choice

Executables, unit tests, and deployments may wish to use a different implementation than that specified by the platform. This can be done by using `CHOOSES_IMPLEMENTATIONS` directive call in the deployment, executable, or unit test's registration. For example, a unit test may wish to choose `Os_File_Stub` as an implementation of `Os_File` to disable platform file system support for the given unit test:

```
register_fprime_ut(
    ...
  CHOOSES_IMPLEMENTATIONS
    Os_File_Stub
)
```

> [!NOTE]
> A CMake target with the name of the chosen implementor *must* be defined somewhere in F´, an F´ library used by the project, or by the project itself.

> [!WARNING]
> Implementations supplying `CONFIGURATION_HEADERS` cannot be chosen this way. This includes every implementation of the OSAL (`Os_File_Stub` above is such an implementation, and the example is an error). See [Implementations with Configuration Headers](#implementations-with-configuration-headers).

## Defining an Implementation

To define an implementation, developers should declare an F´ module (using `register_fprime_module`) that implements the needed interface of the package and supply the `IMPLEMENTS` directive to that call.

## Implementations with Configuration Headers

Some packages are bound to their implementation at compile time: the code using the package includes a configuration header that tells it about the implementation. The OSAL is the canonical example: `Os/Mutex.hpp` includes `config/MutexSelection.hpp`, which names the C++ type implementing `Os::Mutex`. Such a header must match the implementation that is linked.

An implementation supplies these headers with the `CONFIGURATION_HEADERS` directive of `register_fprime_implementation`:

```cmake
register_fprime_implementation(
    Os_Mutex_MyOs
  IMPLEMENTS
    Os_Mutex
  ...
  CONFIGURATION_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/selection/MutexSelection.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/selection/ConditionVariableSelection.hpp"
)
```

When the implementation is the one chosen for its package, its headers are placed in the build cache and included as `config/<header file name>` (e.g. `config/MutexSelection.hpp`). The headers of implementations that are not chosen are not placed. One choice, `CHOOSES_IMPLEMENTATIONS`, therefore selects both what is compiled and what is linked.

The headers are global to the build, which brings the following rules. Each is a fatal error at generation time:

1. An implementation supplying `CONFIGURATION_HEADERS` may only be chosen in `register_fprime_config`. Executables, deployments, and unit tests may not choose it. As before, the last choice made for a package wins, so a project may replace the choice of its platform from its own configuration module.
2. When one implementation of a package supplies `CONFIGURATION_HEADERS`, the implementation chosen for that package must supply them as well.
3. Two chosen implementations may supply a header of the same file name only when the content is identical.
4. A configuration module may not supply a file with the name of a placed header. In particular, these headers cannot be replaced with `CONFIGURATION_OVERRIDES`: choose another implementation instead.

> [!NOTE]
> A unit test needing a specific implementation of such a package should use that implementation directly, by depending on its module and naming its type (e.g. `Os::FileCommon<Os::Stub::File::StubFile>`), rather than choosing it for the executable.

## Conclusion

F´ provides for various implementations for certain packages needed by the framework.
