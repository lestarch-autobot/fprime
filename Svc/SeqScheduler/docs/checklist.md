# F´ (F Prime) Component Development Checklist

**Component Name:** SeqScheduler  
**Component Owner:** _[Owner Name]_  
**Component Contributors:** _[Contributor Names]_  
**Review Date:** 2026-11-19  
**Checklist Version:** 1.2.0

_Pre-filled by fprime-auto-checklist 0.1.0 for `Svc/SeqScheduler` at `5afc29f1df17`. 🟢 YES 62, 🟠 NO 2, ⚪ N/A 5, 🔵 REVIEW 0, ⚫ UNCHECKED 4._
_Section fingerprints (inputs each section judges): 1=d2dfcebe38b8 2=7d17e8545144 3=fe9ace06945a 4=e96eea6632d2 5=34711b44a469_
_Stage: **implementation** (implementation and unit test); sections due: 1, 2, 3, 4_

## Status Legend

| Status | Meaning |
|--------|---------|
| 🟢 YES | Item satisfied -- verified by a tool (or attested by the checked record). |
| 🟠 NO | Not satisfied -- a tool found a violation; see Notes. |
| ⚪ N/A | Not applicable -- the model declares nothing this item concerns. |
| 🔵 REVIEW | Needs a human or LLM judgement; the rule to apply is in Notes. |
| ⚫ UNCHECKED | The deciding tool did not run (missing input, tool or license ack); see Notes. |

**Note:** Any status other than **🟢 YES** requires an explanation in the **Notes** column.

## Definitions

| Term | Definition |
|------|------------|
| SDD | Software Description Document — the design documentation for the component. |
| Block diagram | A diagram illustrating the component's ports (and their directions/types), used to visualize the component's interfaces. |

_clang-tidy: 44 rules run; 0 finding(s) in autocoded files were set aside (autocoder, not component)._

## 1. Requirements

| # | Item | Status | Notes |
|---|------|--------|-------|
| 1.1 | Do requirements exist in the component SDD that define the component's expected operational behaviors? | 🟢 YES |  |
| 1.2 | Do requirements exist in the component SDD that define input data expected to be received and handled by the component? | 🟢 YES |  |
| 1.3 | Do requirements exist in the component SDD that define output data expected to be produced by the component? | 🟢 YES |  |
| 1.4 | Do requirements exist in the component SDD for all relevant memory usage constraints for the component? | 🟢 YES |  |
| 1.5 | Do requirements exist in the component SDD for all relevant hardware I/O usage constraints for the component? | ⚪ N/A | No hardware I/O: no driver port; files go through the OS file abstraction (SDD 4.2). |
| 1.6 | Do requirements exist in the component SDD for all relevant runtime performance constraints for the component? | ⚪ N/A | The request gives no throughput or latency figure; timing is set by the schedule tick (REQ-010) and LATE_TOLERANCE_SEC (REQ-021, REQ-044). |
| 1.7 | Does every requirement specify a verification method? | 🟢 YES | R8 pass (56 requirement(s) checked) |
| 1.8 | Are there any relevant external interface or functional requirements documented elsewhere such as ICDs/FDDs? If so, please specify in notes. | 🟢 YES | D9 pass (external references table: None (declared)) |

## 2. Design

| # | Item | Status | Notes |
|---|------|--------|-------|
| 2.1 | Is the high level component functionality documented/described in the SDD? | 🟢 YES | SDD pass (tier gold) |
| 2.2 | Is the component block diagram documented in the SDD and consistent with the FPP model? | 🟢 YES | D9 pass (external references table: None (declared)) |
| 2.3 | Is the component port table documented in the SDD and consistent with the FPP model? | 🟢 YES | D7 pass (7 port(s) checked) |
| 2.4 | Are the component input port behaviors described in the SDD? | 🟢 YES | D10 pass (4 input port(s) checked) |
| 2.5 | Are the component input port handlers defined? | 🟢 YES | D1 pass (4 input port(s) checked) |
| 2.6 | Are the component command handlers defined? | 🟢 YES | D2 pass (8 command(s) checked) |
| 2.7 | Are the component sequence diagrams documented? | 🟢 YES | D9 pass (external references table: None (declared)) |
| 2.8 | Are component state machines and use-cases described in the SDD? | 🟢 YES | D9 pass (external references table: None (declared)) |
| 2.9 | Are the state machine actions defined? | ⚪ N/A | D3: no state machines declared |
| 2.10 | Are component commands, telemetry, events and parameters documented in the SDD (and consistent with the FPP model)? | 🟢 YES | D8 pass (47 dictionary item(s) checked) |
| 2.11 | Are component data products documented and consistent with the FPP model? | 🟢 YES | D8 pass (47 dictionary item(s) checked) |
| 2.12 | Are all data product containers assigned a default priority? | 🟢 YES | D4 pass (1 container(s) checked) |
| 2.13 | Does each component port in the FPP model have a known/planned use-case? | 🟢 YES | R6 pass (7 port(s) traced against 56 requirement(s); 7 covered by companion/SDD evidence)<br>D10 pass (4 input port(s) checked) |
| 2.14 | Are port directions and types as expected? | 🟢 YES | D7 pass (7 port(s) checked) |
| 2.15 | Is the component type as expected (passive, queued or active)? | 🟢 YES | D9 pass (external references table: None (declared)) |
| 2.16 | Are all the component requirements mapped to design description and/or artifacts? | 🟢 YES | D5 pass (56 requirement(s) checked) |
| 2.17 | Does the component FPP compile and generate auto-code? | 🟢 YES | D6 pass (1 FPP file(s) checked) |

## 3. Implementation

| # | Item | Status | Notes |
|---|------|--------|-------|
| 3.1 | Are all component source dependencies listed in the CMakeLists.txt file for the component? | 🟢 YES | I1 pass (3 dependency module(s) checked; 1 provided by FPP/implicit framework dependencies) |
| 3.2 | Does the component compile without warnings for all required targets? | 🟢 YES | I2 pass (1 log(s) scanned) |
| 3.3 | Are all loops properly terminated/bounded? | 🟢 YES | `cpp/fprime/jpl-c/loop-bounds` 0 findings |
| 3.4 | Has gating been provided for any potential cyclic error/status messages (e.g. log messages, events)? | 🟢 YES | `cpp/fprime/unthrottled-cyclic-event` 0 findings |
| 3.5 | Do all switch statements have a break after each case and terminate with a default case? | 🟢 YES | `cpp/fprime/switch-default` 0 findings<br>`cpp/fprime/switch-case-fallthrough` 0 findings |
| 3.6 | Verify all non-empty case statements that fall through to the following case statement are explicitly commented as such | 🟢 YES | `cpp/fprime/switch-case-fallthrough` 0 findings |
| 3.7 | Are all variables locally defined unless global or specific visibility is required? | 🟢 YES | `cpp/fprime/global-mutable-variable` 0 findings |
| 3.8 | Are all class members defined appropriately using private, public and protected keywords? | 🟢 YES | `cpp/fprime/public-data-member` 0 findings |
| 3.9 | Verify that no floating point expressions are tested for equality/inequality | 🟢 YES | `cpp/fprime/float-equality` 0 findings |
| 3.10 | Is the precision of floating point numbers sufficient to ensure accuracy and floating-point equivalence is evaluated with margin (i.e. safe floating-point usage), and is not used in a loop control? | 🟢 YES | `cpp/fprime/float-narrowing` 0 findings<br>`cpp/fprime/float-loop-control` 0 findings<br>`cpp/fprime/float-equality` 0 findings |
| 3.11 | Are all variables initialized before use? | 🟢 YES | `cpp/fprime/uninitialized-member` 0 findings<br>`cpp/uninitialized-local` 0 findings |
| 3.12 | Are the priorities of all data products produced by the component configurable via command(s) or parameter(s)? | 🟠 NO | I3: container 'TableContainer' priority is not configurable: no command or parameter mentioning 'priority' found |
| 3.13 | Are all commands acknowledged with success/failure? | 🟢 YES | `cpp/fprime/command-response` 0 findings |
| 3.14 | Do all validation failure cases for port and command handlers include a return statement to avoid unintended code execution beyond validation failures? | 🟢 YES | `cpp/fprime/validation-failure-return` 0 findings |
| 3.15 | Are all internal port interfaces invoked via the auto-coded internal port invoke() function? | 🟢 YES | `cpp/fprime/internal-port-invoke` 0 findings |
| 3.16 | All pointer arguments to functions are checked to be not nullptr using FW_ASSERT | 🟢 YES | `cpp/fprime/checking-parameter-values` 0 findings |
| 3.17 | Assumptions on software state are validated using FW_ASSERT. | 🟢 YES | `cpp/fprime/jpl-c/use-of-assertions-density` 0 findings |
| 3.18 | Assertions shall not be used to validate state that could be dependent on hardware or user input. | 🟢 YES | `cpp/fprime/assert-on-untrusted-input` 0 findings |
| 3.19 | Verify all ISR code is free of floating point operations | 🟢 YES | `cpp/fprime/no-float-in-isr` 0 findings |
| 3.20 | Has the ISR code been optimized to minimize length/execution time? | 🟢 YES | `cpp/fprime/isr-complexity` 0 findings |
| 3.21 | Verify any software state that's expected upon receive of an interrupt is configured prior to enabling/setting conditions that would allow the interrupt to trigger. | ⚪ N/A | No interrupt service routine in the component. |
| 3.22 | Verify there are no blocking operations in ISRs | 🟢 YES | `cpp/fprime/no-blocking-in-isr` 0 findings |
| 3.23 | Verify any relevant interrupt enables and interrupts are cleared during initialization and re-enabled selectively as needed. | ⚪ N/A | No interrupt is enabled or used by the component. |
| 3.24 | Are RateGroup schedule calls non-blocking? | 🟢 YES | `cpp/fprime/no-blocking-in-sched-handler` 0 findings |
| 3.25 | Verify no synchronous file access is done within a critical region (e.g. SchedIn) | 🟢 YES | `cpp/fprime/no-file-access-in-critical` 0 findings |
| 3.26 | Verify that all loops and if/else statements are compound statements (i.e. always use brackets {}) | 🟢 YES | `cpp/fprime/compound-statements` 0 findings |
| 3.27 | Verify that all divisions first check for small numbers (including zero) | 🟢 YES | `cpp/fprime/division-guard` 0 findings<br>`cpp/fprime/float-division-guard` 0 findings |
| 3.28 | Verify all array accesses are checked to be within array index bounds | 🟢 YES | `cpp/fprime/unchecked-array-index` 0 findings<br>`cpp/fprime/checking-parameter-values` 0 findings |
| 3.29 | Verify there are no function declarations within a function. | 🟢 YES | `cpp/fprime/no-function-decl-in-function` 0 findings |
| 3.30 | Verify that the three expressions in all for statements only concern flow control | 🟢 YES | `cpp/fprime/for-header-flow-control` 0 findings |
| 3.31 | Verify that there are no dependencies on signed integer overflow behavior | 🟢 YES | `cpp/fprime/signed-overflow` 0 findings |
| 3.32 | Verify return values are used or voided otherwise. | 🟢 YES | `cpp/fprime/jpl-c/checking-return-values` 0 findings |
| 3.33 | Verify units are incorporated into variable name, where applicable | 🟢 YES |  |
| 3.34 | Verify that all invariant numbers are in enums, static constexpr or #defines. Enums or static constexpr must be used unless #define is only way to specify constant. | 🟢 YES | `cpp/fprime/magic-numbers` 0 findings<br>`cpp/fprime/define-constant` 0 findings |
| 3.35 | Verify no use of printf | 🟢 YES | `cpp/fprime/no-printf` 0 findings |
| 3.36 | Does the component use platform independent standard data types (e.g. I32, F32 etc.) rather than primitive data types (e.g. int, float etc.)? | 🟢 YES | `cpp/fprime/jpl-c/basic-int-types` 0 findings |
| 3.37 | Does the component pass static analysis? | 🟢 YES | I4 pass (0 finding(s) in module; threshold medium) |
| 3.38 | Does the implementation address all component requirements and remains consistent with the documented design? | 🟢 YES |  |

## 4. Unit Testing

| # | Item | Status | Notes |
|---|------|--------|-------|
| 4.1 | Have all test output/results been published? | ⚫ UNCHECKED | U1 not run: not done yet: no published unit-test artifacts given (--artifacts-dir) |
| 4.2 | Are all component unit test source dependencies listed in the CMakeLists.txt file for the component? | 🟢 YES | U3 pass (5 dependency module(s) checked) |
| 4.3 | Have input data ranges been tested for boundary conditions? | 🟢 YES |  |
| 4.4 | Do the unit tests pass a memory leak check? | ⚫ UNCHECKED | U4 not run: not done yet: the leak check (unit-test executable under LeakSanitizer/valgrind) has not been run |
| 4.5 | Is there sufficient code coverage? | 🟢 YES | U5 pass (line coverage 98.48% (threshold 80.00%)) |

## 5. Close-Out

| # | Item | Status | Notes |
|---|------|--------|-------|
| 5.1 | Verify no TBDs, TODOs or printfs in the code (no incomplete code). | 🟢 YES | `cpp/fprime/incomplete-code` 0 findings<br>`cpp/fprime/no-printf` 0 findings |
| 5.2 | Verify that component can be instantiated, connected, and compiled along with the Topology for all applicable targets. | ⚫ UNCHECKED | C1 not run: not done yet: no topology build performed by this run (pass --deployment with --build, or the process builds the deployment) |
| 5.3 | Verify that the flight deployment with component instantiation/connections runs successfully on a hardware platform. Indicate hardware platform used for checkout in Notes. | 🟠 NO | Human: Run the flight deployment that instantiates this component on the target hardware and record platform, deployment, commit, date, result and tester in docs/hardware-run.md (check C2 validates the record).<br>C2: no docs/hardware-run.md: record the hardware run (Platform, Deployment, Commit, Date, Result, Tester) |
| 5.4 | Update the SDD to accurately reflect the final state of the component. Add any additional design detail, including any known idiosyncrasies. | 🟢 YES | D7 pass (7 port(s) checked)<br>D8 pass (47 dictionary item(s) checked)<br>D9 pass (external references table: None (declared))<br>SDD pass (tier gold) |
| 5.5 | Validate that CI passes. | ⚫ UNCHECKED | C3 not run: not done yet: CI status not consulted (needs the repository, the commit and GITHUB_TOKEN) |

## Hand-off

Nothing to review at the **implementation** stage.
