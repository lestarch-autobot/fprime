# Develop Components in Rust

This guide shows how to implement F Prime components in Rust and build and run a deployment that contains them.

[`fprime-rust`](https://github.com/lestarch-autobot/fprime-rust) is an F Prime autocoder plugin with a small Rust runtime. It autocodes the bindings between a component's C++ base class and an implementation written in Rust, so that the rest of the system calls the Rust component like any other component. The FPP model stays the source of truth: Rust replaces only the hand-written `.cpp` and `.hpp` of the component.

For a complete project, see the [F Prime Rust Reference](https://github.com/lestarch-autobot/fprime-rust-reference).

---

## Contents

- [When to Use This](#when-to-use-this)
- [Why Rust](#why-rust)
- [Prerequisites](#prerequisites)
- [Rust in F Prime](#rust-in-f-prime)
- [One Rust Runtime per Executable](#one-rust-runtime-per-executable)
- [Project Setup](#project-setup)
- [Create the Component](#create-the-component)
- [Set Up the Deployment](#set-up-the-deployment)
- [Build and Run](#build-and-run)
- [Passing Data Between Rust and F Prime](#passing-data-between-rust-and-f-prime)
- [Testing](#testing)
- [Troubleshooting](#troubleshooting)
- [References](#references)

## When to Use This

Use this guide when you:

- Have existing Rust code that you wish to integrate into an F Prime project
- Want the memory safety of Rust for a component without leaving the F Prime component model
- Want to explore F Prime development through Rust

## Why Rust

Reasons to implement a component in Rust rather than C++:

1. Rust enforces memory and thread safety at compile time without a garbage collector, so a component keeps the deterministic, allocation-free execution F Prime expects while ruling out whole classes of memory errors.
2. Rust compiles to a static library linked into the deployment executable: there is no interpreter, no extension module, and no change to how the deployment starts or runs.
3. The generated bindings expose the component's commands, events, telemetry, parameters, and ports as plain Rust methods, so the implementation reads like the C++ one.

Prefer C++ for a component whose interface uses types `fprime-rust` does not yet support (structs, arrays, `Fw.Buffer`, see [Rust in F Prime](#rust-in-f-prime)), or that must share code with existing C++ libraries. The FPP model does not change when a component moves between the two languages.

> [!CAUTION]
> `fprime-rust` is a young project (version 0.1.0) and has not been through a flight qualification. Its runtime aborts the flight software on a Rust panic, by design, through F Prime's assertion hook. Evaluate with care before electing to use `fprime-rust` on a flight system.

## Prerequisites

- A working F Prime development environment and an F Prime project
- Python 3.9 or newer in the virtual environment used to build the project. This guide was verified with Python 3.12.
- A Rust toolchain installed with [`rustup`](https://rustup.rs), providing `cargo` and `rustc` on the `PATH`. This guide was verified with the stable toolchain, `rustc`/`cargo` 1.99.1:

    ```bash
    curl https://sh.rustup.rs -sSf | sh -s -- -y && source ~/.cargo/env
    cargo --version
    ```

- A C++ toolchain for the native build. This guide was verified on Ubuntu 22.04 x86_64 with GCC 11.
- Linux x86_64, where this guide was verified. `fprime-rust` reads the FPP model with [`fprime-fpp-python`](https://pypi.org/project/fprime-fpp-python/), which publishes wheels for Linux x86_64, macOS arm64, and Windows x64 only.
- Basic familiarity with FPP, topologies, and Rust development

## Rust in F Prime

A component marked for Rust is modeled in FPP, registered in CMake, and implemented in a single `<Component>.rs` file. The build does the rest:

1. For each component annotated `@ fprime-rust`, the `fprime-rust-ac` autocoder reads the FPP model and generates a C++ *shim* and a Rust *base module* into the component's build directory. The shim derives from the autocoded `<Component>ComponentBase` and forwards every handler across an `extern "C"` boundary. The base module declares a `Component` trait with one method per handler and a `Context` type giving the implementation its output ports, events, telemetry channels, and parameters.
2. For each executable that depends on at least one Rust component, the build generates a Cargo workspace with one library crate per component and one static-library crate that re-exports them all, builds it with `cargo build`, and links the resulting archive into the executable together with the `fprime-rust` runtime.

Components written in C++ run unchanged beside those written in Rust, and the ground system cannot tell them apart: the Rust component has the same commands, events, channels, and parameters, under the same names, as it would in C++.

For each Rust component the build writes these files into its build directory, `build-fprime-automatic-native/<path to component>/`, next to the `<Component>ComponentAc.*` files. None of them is edited by hand:

| Generated file | Contents |
| --- | --- |
| `<Component>.hpp`, `<Component>.cpp` | The C++ shim: a `final` class deriving from `<Component>ComponentBase` whose handlers call into Rust. The topology instantiates this class under the component's usual name. |
| `<component>_base.rs` | The Rust base module: the `Component` trait, `Context`, `InitContext`, and the `extern "C"` entry points. The lower-case component name is the module name. |
| `<Component>.template.rs` | A complete stub implementation of the trait, regenerated whenever the model changes. |

A Rust component may not use the following. Each is rejected at build time with an error naming the offender and the reason:

- Struct and array types, and `Fw.Buffer` or `Fw.Com` arguments
- Serial ports
- State machine instances
- Data product containers
- `ref` port parameters
- A `string` without a `size` as a parameter or port return value

## One Rust Runtime per Executable

Every Rust static library embeds the Rust standard library and runtime it was built against. Were each component built into its own archive, a deployment with several Rust components would link several copies of the Rust runtime: duplicate symbols at best, and a larger footprint at least. `fprime-rust` therefore builds **one Cargo workspace and one static archive per executable**, containing every Rust component the executable depends on, and links the runtime exactly once.

This is handled by the `fprime_rust` build target that `include(FprimeRust)` registers (see [Project Setup](#project-setup)):

1. When the build cache is generated, the target walks each executable's dependency list and collects the modules whose sources include a `.rs` file. An executable with no Rust component is left alone; `cargo` never runs for it.
2. For each executable with Rust components it writes a workspace to `<build cache>/<path to executable>/fprime_rust/<EXECUTABLE>/`, with one `rlib` member per component and a `staticlib` root crate, and arranges for `cargo build` to produce `libfprime_rust_<EXECUTABLE>.a`.
3. It links the executable so that the shim objects precede the archive and the runtime library follows it.

The reference deployment has two Rust components, `RustCounter` (active) and `RustMonitor` (queued), beside two C++ components. Generating its build cache reports the single archive it links:

```
-- [fprime_rust] cargo: /home/ubuntu/.cargo/bin/cargo; target: x86_64-unknown-linux-gnu; profile: release
-- [fprime_rust] FprimeRustReference_ReferenceDeployment links .../build-fprime-automatic-native/FprimeRustReference/ReferenceDeployment/fprime_rust/FprimeRustReference_ReferenceDeployment/target/x86_64-unknown-linux-gnu/release/libfprime_rust_FprimeRustReference_ReferenceDeployment.a
```

and the build compiles both components into it:

```
[fprime_rust] cargo build of FprimeRustReference_ReferenceDeployment Rust components
   Compiling fprimerustreference_rustcounter v0.1.0 (...)
   Compiling fprimerustreference_rustmonitor v0.1.0 (...)
   Compiling fprime_rust_FprimeRustReference_ReferenceDeployment v0.1.0 (...)
```

Unit-test executables are executables too: each `<Module>_ut_exe` gets its own workspace and archive, so a unit test links the components it tests and nothing else.

Nothing in a project configures this. Do not add a `Cargo.toml` to a component, do not link a Rust archive yourself, and do not list the generated `.rs` or `.cpp` files in CMake: the only Rust input the build takes is `<Component>.rs`.

> [!TIP]
> The workspace is generated with a `release` profile, link-time optimization, `panic = "abort"`, and a set of `deny` lints (`unwrap_used`, `expect_used`, `indexing_slicing`, `arithmetic_side_effects`, `unsafe_op_in_unsafe_fn`). Write the component accordingly: use checked or saturating arithmetic and avoid `unwrap()`.

## Project Setup

1. Add `fprime-rust` to the project as a submodule:

    ```bash
    git submodule add https://github.com/lestarch-autobot/fprime-rust.git lib/fprime-rust
    ```

2. List it as an F Prime library in the project's `settings.ini`. Separate multiple libraries with `:`.

    ```ini
    [fprime]
    library_locations: ./lib/fprime-rust
    ```

3. Install it into the project's virtual environment, after the F Prime requirements. This installs the `fprime-rust-ac` autocoder with its dependencies, `fprime-fpp-python` and `fprime-cpp-codegen`:

    ```bash
    pip install -r lib/fprime/requirements.txt
    pip install -e lib/fprime-rust
    ```

4. In the project's top-level `CMakeLists.txt`, include `FprimeRust` after `FPrime.cmake` and before `fprime_setup_included_code()`:

    ```cmake
    include("${CMAKE_CURRENT_LIST_DIR}/lib/fprime/cmake/FPrime.cmake")
    include(FprimeRust)
    fprime_setup_included_code()
    ```

    This registers the `fprime_rust` autocoder and build target for normal and unit-test builds, locates `cargo`, and chooses the Cargo target triple and profile. Override the location of `cargo` with `-DFPRIME_RUST_CARGO=<path>` when it is not on the `PATH`.

5. Generate the build cache from the deployment's directory:

    ```bash
    fprime-util generate
    ```

    The output confirms the toolchain `fprime-rust` will use: `[fprime_rust] cargo: ...; target: ...; profile: release`.

## Create the Component

A Rust component consists of an FPP model, a Rust implementation, and a `CMakeLists.txt`. It has no C++ implementation files. Create it in either of two ways.

**From the template (recommended).** Point `fprime-util new` at the Rust component cookiecutter in `settings.ini`:

```ini
[fprime]
component_cookiecutter: ./lib/fprime-rust/cookiecutters/component
```

Then, from the directory that will hold the component, run:

```bash
fprime-util new --component
```

Answer the prompts for the component's name, description, namespace, and kind (active, passive, or queued), and add the component to the build when asked. Answer **no** to `Generate implementation files?`: that step generates C++ implementation files, which a Rust component does not have. The new component is complete: its model is the one shown [below](#model-the-component), with the name you chose. Continue at [Set Up the Deployment](#set-up-the-deployment), and return to the following sections when changing the component.

The cookiecutter can also be run directly, from the root of the project: `cookiecutter lib/fprime-rust/cookiecutters/component`. Add the component's directory to the enclosing `CMakeLists.txt` yourself in that case.

**From a C++ component.** Run `fprime-util new --component` with the default cookiecutter, then:

1. Delete the generated `<Component>.cpp` and `<Component>.hpp` files from disk.
2. Remove both files from the `SOURCES` block of the component's `CMakeLists.txt`.
3. Annotate the model, implement it, and register the Rust file as described below.

### Model the Component

Annotate the component with `@ fprime-rust` on its own line directly above the component definition. Everything else is modeled as for a C++ component, including the standard ports that commands, events, telemetry, and parameters require.

```fpp
module FprimeRustReference {
    @ Greets from Rust
    @ fprime-rust
    active component HelloRust {

        @ Place-holder command: reports the greeting it is sent as a Hello event
        async command HELLO(
            greeting: string size 40 @< Greeting to report
        )

        @ Place-holder event: the greeting received by the HELLO command
        event Hello(
            greeting: string size 40 @< Greeting received
        ) severity activity high format "Hello: {}"

        @ Place-holder telemetry channel: count of HELLO commands received
        telemetry HelloCount: U32

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending command registrations
        command reg port cmdRegOut

        @ Port for receiving commands
        command recv port cmdIn

        @ Port for sending command responses
        command resp port cmdResponseOut

        @ Port for sending textual representation of events
        text event port logTextOut

        @ Port for sending events to downlink
        event port logOut

        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut

        @ Port to return the value of a parameter
        param get port prmGetOut

        @ Port to set the value of a parameter
        param set port prmSetOut
    }
}
```

### Implement the Component

1. Register the model and the Rust file in the component's `CMakeLists.txt`. No C++ file belongs there:

    ```cmake
    register_fprime_module(
        AUTOCODER_INPUTS
            "${CMAKE_CURRENT_LIST_DIR}/HelloRust.fpp"
        SOURCES
            "${CMAKE_CURRENT_LIST_DIR}/HelloRust.rs"
    )
    ```

    List the modules whose ports or types the model uses under `DEPENDS`, as for a C++ component.

2. Build the project:

    ```bash
    fprime-util build
    ```

    Every build writes `<Component>.template.rs` into the component's build directory, `build-fprime-automatic-native/<path to component>/`, with a stub for each handler in the model. It replaces `fprime-util impl`.

    `<Component>.rs` must exist for the build to configure, since it is listed in `SOURCES`. The cookiecutter provides one; when converting a C++ component, create an empty one, build once to obtain the template, and copy the template over it. After a later change to the model, copy the stubs of the new handlers from the template into `<Component>.rs` instead.

3. Implement the handlers in `<Component>.rs`. The file is compiled as the root of the component's crate, so it starts by importing the generated base module, named after the component in lower case:

    ```rust
    //! `FprimeRustReference.HelloRust`: Greets from Rust
    use crate::hellorust_base::*;

    #[derive(Default)]
    pub struct HelloRust {
        hello_count: u32,
    }

    impl Component for HelloRust {
        fn new() -> Self {
            Self::default()
        }

        fn HELLO_cmd_handler(&mut self, ctx: &mut Context<'_>, greeting: &str) -> CmdResponse {
            self.hello_count = self.hello_count.wrapping_add(1);
            ctx.log_ACTIVITY_HI_Hello(greeting);
            ctx.tlmWrite_HelloCount(self.hello_count);
            CmdResponse::Ok
        }
    }
    ```

    The struct holds the component's state; the trait methods are the handlers. Each handler receives `&mut self` and a `Context` for the duration of the call. The names follow the C++ ones:

    | FPP element | Rust |
    | --- | --- |
    | `async`/`sync`/`guarded` input port `p` | `fn p_handler(&mut self, ctx: &mut Context<'_>, port_num: FwIndexType, <args>)` |
    | Output port `p` | `ctx.p_out(port_num, <args>)` |
    | Command `C` | `fn C_cmd_handler(&mut self, ctx: &mut Context<'_>, <args>) -> CmdResponse`; the response is sent for you |
    | Event `E` of severity `S` | `ctx.log_S_E(<args>)`, e.g. `ctx.log_WARNING_LO_Saturated(value, threshold)` |
    | Telemetry channel `T` | `ctx.tlmWrite_T(value)` |
    | Parameter `P` | `ctx.paramGet_P() -> (value, ParamValid)`; `fn parameter_updated(&mut self, ctx: &mut Context<'_>, id: FwPrmIdType)` is called after a `P_PRM_SET` command |
    | Command `C` with an overflow `hook` | `fn C_overflow_hook(ctx: &mut HookContext<'_>, opcode: FwOpcodeType, cmd_seq: u32) -> CmdResponse` |

    `Component::new()` builds the state when the topology constructs the component; `init(&mut self, ctx: &InitContext<'_>)` runs during component initialization. `new` is required; `init` and `parameter_updated` have empty default implementations.

    > [!CAUTION]
    > `new` and `init` run while the topology initializes its components, before any port is connected. Initialize Rust state there, but do not send events, write telemetry, or call ports: `InitContext` deliberately offers none of them.

4. Rebuild. Editing `<Component>.rs` alone needs no `fprime-util generate`; `fprime-util build` runs `cargo build` for every executable that contains the component.

### Queued Components

A queued Rust component drains its message queue whenever one of its `sync` or `guarded` inputs is called, before that input's handler runs, so that a thread of the deployment delivers the queued messages. Model one such input, typically a `Svc.Sched` port driven by a rate group, and make the other inputs `async`. The reference `RustMonitor` is queued:

```fpp
    @ fprime-rust
    queued component RustMonitor {
        @ Drains the queue once per cycle
        sync input port run: Svc.Sched

        @ Count to check against the limit; dropped when the queue is full
        async input port countIn: FprimeRustReference.Count drop

        @ Set the count limit; zero is rejected; on a full queue the overflow hook responds BUSY
        async command SET_LIMIT(limit: U32) opcode 0 hook
```

Its handlers show a command rejecting an argument, and the overflow hook for a `hook` command:

```rust
    fn SET_LIMIT_cmd_handler(&mut self, ctx: &mut Context<'_>, limit: u32) -> CmdResponse {
        if limit == 0 {
            ctx.log_WARNING_LO_InvalidLimit(limit);
            return CmdResponse::ValidationError;
        }
        self.limit = limit;
        ctx.log_ACTIVITY_LO_LimitSet(limit);
        ctx.tlmWrite_Limit(limit);
        CmdResponse::Ok
    }

    fn SET_LIMIT_overflow_hook(
        _ctx: &mut HookContext<'_>,
        _opcode: FwOpcodeType,
        _cmd_seq: u32,
    ) -> CmdResponse {
        CmdResponse::Busy
    }
```

`Context::last_dispatch()` reports the status of the drain that preceded the call, and `Context::num_msgs_dropped()` the messages dropped so far.

Synchronous connections between Rust components must not form a cycle. Break a cycle with an `async` port, as the reference does: `rustCounter.countOut` reaches `rustMonitor.countIn`, an `async` port, through the C++ `relay` component.

## Set Up the Deployment

A Rust component is instantiated and connected like any other, and the deployment needs no change beyond that: no annotation, no change to `Main.cpp`, no change to the deployment's `CMakeLists.txt`. The build detects the Rust components among an executable's dependencies by itself.

1. **Instantiate the component.** Define the instance in `Top/instances.fpp`:

    ```fpp
    instance helloRust: FprimeRustReference.HelloRust base id 0x10030000 \
      queue size Default.QUEUE_SIZE \
      stack size Default.STACK_SIZE \
      priority 17
    ```

    Then add it to the topology in `Top/topology.fpp`. Without this line the component is not part of the deployment:

    ```fpp
    instance helloRust
    ```

2. **Connect it.** Connect its ports in `Top/topology.fpp` as for a C++ component. The standard command, event, telemetry, parameter, and time ports connect automatically through the `command`, `event`, `telemetry`, `param`, and `time` connection graphs. The reference connects a rate group to the `tick` port of `rustCounter` and to the `run` port of `rustMonitor`.

3. **List its channels.** If the deployment defines telemetry packets (`Top/Packets.fppi` in the reference), add the component's channels to a packet or to the `omit` list, as for any component. Otherwise the build fails with `error: invalid telemetry packet set Packets`.

## Build and Run

The commands below run in the reference project, from a clean clone, and were used to verify this guide. Replace the paths for your own project.

1. Clone the reference with its submodules, create the virtual environment, and generate the build cache:

    ```bash
    git clone --recurse-submodules https://github.com/lestarch-autobot/fprime-rust-reference.git
    cd fprime-rust-reference
    python3.12 -m venv venv && source venv/bin/activate
    pip install -r lib/fprime/requirements.txt
    pip install -e lib/fprime-rust
    cd FprimeRustReference/ReferenceDeployment
    fprime-util generate
    ```

2. Build from the deployment's directory:

    ```bash
    fprime-util build -j8
    ```

    The build runs the autocoder (`[fprime_rust] Generating Rust bindings for ...`), then `cargo build` for the deployment, then installs the executable and dictionary to `build-artifacts/<platform>/<deployment>/`:

    ```
    build-artifacts/Linux/FprimeRustReference_ReferenceDeployment/bin/FprimeRustReference_ReferenceDeployment
    build-artifacts/Linux/FprimeRustReference_ReferenceDeployment/dict/FprimeRustReferenceTopologyDictionary.json
    ```

    For a deployment below the root of the project, the directory after `<platform>` is the deployment's path with `/` replaced by `_`.

3. Start the GDS from the root of the project. It launches the flight software and serves its GUI at `http://127.0.0.1:5000`:

    ```bash
    cd ../..
    fprime-gds --deployment build-artifacts/Linux/FprimeRustReference_ReferenceDeployment
    ```

4. Send commands to the Rust components from the GDS GUI, or with `fprime-cli` from a second terminal in the same virtual environment:

    ```bash
    DICT=$PWD/build-artifacts/Linux/FprimeRustReference_ReferenceDeployment/dict/FprimeRustReferenceTopologyDictionary.json
    fprime-cli command-send FprimeRustReference.rustCounter.RESET --dictionary $DICT
    fprime-cli command-send FprimeRustReference.rustCounter.BUMP --arguments 5 --dictionary $DICT
    fprime-cli command-send FprimeRustReference.rustCounter.THRESHOLD_PRM_SET --arguments 42 --dictionary $DICT
    fprime-cli command-send FprimeRustReference.rustMonitor.SET_LIMIT --arguments 3 --dictionary $DICT
    ```

    The Events tab shows `rustCounter.ResetIssued`, `rustCounter.Bumped` (`RustCounter bumped to 5`), `rustMonitor.LimitExceeded` (`count 5 exceeds limit 3`), and `rustMonitor.LimitSet`; the Channels tab shows `rustCounter.Counter` at 5, `rustCounter.ThresholdValid` as `VALID`, `rustMonitor.Limit` at 3, and `TickCount`/`RunCount` advancing once a second. `THRESHOLD_PRM_SET` is the parameter-set command FPP generates for the `THRESHOLD` parameter; the Rust implementation reads the new value with `ctx.paramGet_THRESHOLD()`.

    To watch the same from the command line, start `fprime-cli events --dictionary $DICT` and `fprime-cli channels --dictionary $DICT` before sending the commands: they report what arrives after they connect.

    A component created from the template answers its `HELLO` command the same way. With `helloRust` added to the deployment as in [Set Up the Deployment](#set-up-the-deployment):

    ```bash
    fprime-cli command-send FprimeRustReference.helloRust.HELLO --arguments "hello from Rust" --dictionary $DICT
    ```

    reports the `Hello` event `Hello: hello from Rust` and bumps `HelloCount`.

5. Stop the GDS with CTRL-C. Before starting it again, make sure the flight software it launched is gone (`pgrep -af FprimeRustReference_ReferenceDeployment`) and that port 5000 is free.

## Passing Data Between Rust and F Prime

- FPP numeric types map to the Rust type of the same width and signedness: `U8`..`U64` to `u8`..`u64`, `I8`..`I64` to `i8`..`i64`, `F32`/`F64` to `f32`/`f64`, `bool` to `bool`. Type aliases resolve to their underlying type.
- An FPP enum maps to a Rust enum of the same name. Two enums with the same simple name in different modules cannot both be used by one component.
- Strings arrive in handlers as `&str` and are passed to events, telemetry, and output ports as `&str`. The shim copies them into the F Prime string of the declared size: a string that does not fit is truncated to that size, and at the first NUL character it contains. A `string` parameter or port return value must declare a `size`, which becomes the Rust buffer length.
- `CmdResponse` mirrors `Fw::CmdResponse` (`Ok`, `InvalidOpcode`, `ValidationError`, `FormatError`, `ExecutionError`, `Busy`); `ParamValid` mirrors `Fw::ParamValid`.
- `Context` borrows the shim's stack frame for one call: it is neither `Send` nor `Sync`, and cannot be stored in the component's state. Store values, not contexts.
- A panic in a handler is not recoverable: the runtime's panic hook forwards the message and location to F Prime's assertion handler, which ends the flight software. Validate inputs and use checked arithmetic instead of letting a panic happen.

## Testing

**Unit tests.** A Rust component is unit tested with the standard C++ test harness: `register_fprime_ut` with a `Tester` class deriving from the autocoded `<Component>GTestBase`. The tester sends commands and invokes ports on the shim, which runs the Rust handlers, and the usual `ASSERT_EVENTS_*`, `ASSERT_TLM_*`, and `ASSERT_from_*` macros check the results. The reference's `RustCounter` registers its tests beside the module:

```cmake
register_fprime_ut(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/RustCounter.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/RustCounterTester.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/RustCounterTestMain.cpp"
    DEPENDS
        FprimeRustReference_Components_RustCounter
        STest
    UT_AUTO_HELPERS
)
```

and drives the Rust command handler through the queue:

```cpp
this->paramSet_THRESHOLD(100, Fw::ParamValid::VALID);
this->component.loadParameters();
this->sendCmd_BUMP(0, 10, 5);
this->component.doDispatch();
ASSERT_EVENTS_Bumped(0, 5);
```

Build and run the unit tests of the reference components from their directory:

```bash
cd FprimeRustReference/Components
fprime-util generate --ut
fprime-util check --all -j8
```

Four test executables run; the two that test Rust components each link their own `libfprime_rust_<Module>_ut_exe.a`, while the C++ ones run no `cargo` at all.

> [!CAUTION]
> The unit-test build installs into the same `build-artifacts/` directory as the deployment build and replaces the deployment executable there with its own, unit-test-flavored one. After running `fprime-util check`, run `fprime-util build` from the deployment again before starting the GDS, or launch the copy left in `build-fprime-automatic-native/bin/<platform>/`.

**Integration tests.** From the ground, a Rust component is indistinguishable from a C++ one, so the [GDS integration test API](../../user-manual/gds/gds-test-api-guide.md) applies unchanged: with the flight software and the GDS running as [above](#build-and-run), run `pytest <test directory> --dictionary <path to dictionary>`. The reference keeps its system tests in `FprimeRustReference/ReferenceDeployment/test/int/`; they are written for a test harness that is not part of the reference, so they are not run with plain `pytest`.

## Troubleshooting

| Symptom | Cause |
| --- | --- |
| `[fprime_rust] cargo not found: set FPRIME_RUST_CARGO to the cargo executable` at `fprime-util generate` | No Rust toolchain on the `PATH`. Install one with `rustup`, run `source ~/.cargo/env`, or pass `-DFPRIME_RUST_CARGO=<path to cargo>`. |
| `[fprime_rust] fprime-rust-ac failed for <module>` | The autocoder is not installed in the active virtual environment (`pip install -e lib/fprime-rust`), or the model uses an element `fprime-rust` rejects: the message that follows names it, `unsupported element <name>: <reason>`. |
| `ModuleNotFoundError: No module named 'fprime'` while processing `settings.ini` | CMake found a `python3` without the F Prime tools. Activate the project's virtual environment and install `lib/fprime/requirements.txt` into it. |
| `error: invalid telemetry packet set Packets` | A new channel is not listed in the deployment's telemetry packets or `omit` list. |
| `` error[E0425]: cannot find type `<Component>` in module `crate::<component>` `` from `cargo build` | `<Component>.rs` is empty or does not define the struct the base module expects. Copy `<Component>.template.rs` over it. |
| `<Component>.template.rs` lacks a new handler | The template is regenerated when the `.fpp` changes; make sure the model was saved and rebuild. |
| The deployment executable in `build-artifacts/` is many times larger than expected, or behaves like a test build | A `fprime-util check` ran after the deployment build and replaced it; rebuild the deployment. |
| `fprime-cli events` shows no events for commands already sent | The command-line tools report only what arrives after they connect. Start them first, or use the GDS GUI. |
| The flight software aborts with an assertion naming a Rust source location | A Rust handler panicked; the runtime turned the panic into an F Prime assertion. |

## References

- [F Prime Rust](https://github.com/lestarch-autobot/fprime-rust): autocoder, runtime, and the [software design document](https://github.com/lestarch-autobot/fprime-rust/blob/main/docs/sdd.md) with the type table and requirements
- [F Prime Rust Reference](https://github.com/lestarch-autobot/fprime-rust-reference)
- [`fprime-rust` runtime crate](https://github.com/lestarch-autobot/fprime-rust/blob/main/runtime/README.md)
- [Develop Components in Python](./python-development.md): the same approach for Python
- [Unit Testing](../../user-manual/overview/unit-testing.md)
- [The Rust Programming Language](https://doc.rust-lang.org/book/) and [`rustup`](https://rustup.rs)
