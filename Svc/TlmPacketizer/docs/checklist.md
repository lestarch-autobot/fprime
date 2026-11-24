# F´ (F Prime) Component Development Checklist

**Component Name:** TlmPacketizer  
**Component Owner:** _[Owner Name]_  
**Component Contributors:** _[Contributor Names]_  
**Review Date:** 2026-11-24  
**Checklist Version:** 1.2.0

_Pre-filled by fprime-auto-checklist 0.2.2 for `Svc/TlmPacketizer` at `c14058e29e9e`. 🟢 YES 43, 🟠 NO 20, ⚪ N/A 6, 🔵 REVIEW 0, ⚫ UNCHECKED 4._
_Section fingerprints (inputs each section judges): 1=9b3a424c0c26 2=82b5399a26ec 3=cd1e24d2e309 4=128782f5aed9 5=106da1b9e674_
_Stage: **implementation** (implementation and unit test); sections due: 1, 2, 3, 4_

## Status Legend

| Status | Meaning |
|--------|---------|
| 🟢 YES | Item satisfied -- verified by a tool (or attested by the checked record). |
| 🟠 NO | Not satisfied -- a tool found a violation; see Notes. |
| ⚪ N/A | Not applicable -- the model declares nothing this item concerns. |
| 🔵 REVIEW | Needs a human or LLM judgement; the rule to apply is in Notes. |
| ⚫ UNCHECKED | The deciding tool did not run (missing input, tool or license ack); see Notes. |

**Note:** Any status other than **🟢 YES** requires an explanation in the **Notes** column. A Status cell reads `<status> @<commit>`: decided by the tool at that commit; `by review` when a reviewer decided a row the tool left open; `overridden by reviewer` when a reviewer replaced the tool's verdict (reason in Notes).

## Definitions

| Term | Definition |
|------|------------|
| SDD | Software Description Document — the design documentation for the component. |
| Block diagram | A diagram illustrating the component's ports (and their directions/types), used to visualize the component's interfaces. |

_clang-tidy: 44 rules run; 0 finding(s) in autocoded files were set aside (autocoder, not component)._

## 1. Requirements

| # | Item | Status | Notes |
|---|------|--------|-------|
| 1.1 | Do requirements exist in the component SDD that define the component's expected operational behaviors? | 🟢 YES @c14058e29e9e by review |  |
| 1.2 | Do requirements exist in the component SDD that define input data expected to be received and handled by the component? | 🟢 YES @c14058e29e9e by review |  |
| 1.3 | Do requirements exist in the component SDD that define output data expected to be produced by the component? | 🟢 YES @c14058e29e9e by review |  |
| 1.4 | Do requirements exist in the component SDD for all relevant memory usage constraints for the component? | 🟢 YES @c14058e29e9e by review |  |
| 1.5 | Do requirements exist in the component SDD for all relevant hardware I/O usage constraints for the component? | ⚪ N/A @c14058e29e9e by review | No hardware port or driver in the FPP model; SDD 3.1 lists only framework ports. |
| 1.6 | Do requirements exist in the component SDD for all relevant runtime performance constraints for the component? | 🟠 NO @c14058e29e9e by review | No requirement bounds setPacketList or TlmRecv run time (TPK-005 covers send rate only); pre-existing, outside issue 6431's one-requirement scope; for the approver. |
| 1.7 | Does every requirement specify a verification method? | 🟢 YES @c14058e29e9e | R8 pass (13 requirement(s) checked) |
| 1.8 | Are there any relevant external interface or functional requirements documented elsewhere such as ICDs/FDDs? If so, please specify in notes. | 🟢 YES @c14058e29e9e | D9 pass (no failures concern this item) |

## 2. Design

| # | Item | Status | Notes |
|---|------|--------|-------|
| 2.1 | Is the high level component functionality documented/described in the SDD? | 🟢 YES @c14058e29e9e | SDD pass (tier gold) |
| 2.2 | Is the component block diagram documented in the SDD and consistent with the FPP model? | 🟢 YES @c14058e29e9e | D9 pass (no failures concern this item) |
| 2.3 | Is the component port table documented in the SDD and consistent with the FPP model? | 🟠 NO @c14058e29e9e | D7: port 'PktSend' (output output Fw.Com) is missing from the SDD port table<br>D7: port 'Run' (async input Svc.Sched) is missing from the SDD port table<br>D7: port 'TlmGet' (sync input Fw.TlmGet) is missing from the SDD port table<br>D7: port 'TlmRecv' (sync input Fw.Tlm) is missing from the SDD port table<br>D7: port 'configureSectionGroupRate' (async input ConfigureGroupRate) is missing from the SDD port table<br>D7: port 'controlIn' (async input EnableSection) is missing from the SDD port table<br>... 15 more |
| 2.4 | Are the component input port behaviors described in the SDD? | 🟠 NO @c14058e29e9e | D10: input port 'pingIn' is not described in SDD prose (only in a table, or absent) |
| 2.5 | Are the component input port handlers defined? | 🟢 YES @c14058e29e9e | D1 pass (8 input port(s) checked) |
| 2.6 | Are the component command handlers defined? | 🟢 YES @c14058e29e9e | D2 pass (9 command(s) checked) |
| 2.7 | Are the component sequence diagrams documented? | 🟠 NO @c14058e29e9e | D9: no sequence diagram (add a mermaid sequenceDiagram, or state 'Sequence diagrams: N/A' with rationale) |
| 2.8 | Are component state machines and use-cases described in the SDD? | 🟢 YES @c14058e29e9e | D9 pass (no failures concern this item) |
| 2.9 | Are the state machine actions defined? | ⚪ N/A @c14058e29e9e | D3: no state machines declared |
| 2.10 | Are component commands, telemetry, events and parameters documented in the SDD (and consistent with the FPP model)? | 🟠 NO @c14058e29e9e | D8: no command table in SDD (9 declared in FPP)<br>D8: no telemetry channel table in SDD (2 declared in FPP)<br>D8: no event table in SDD (16 declared in FPP)<br>D8: no parameter table in SDD (2 declared in FPP) |
| 2.11 | Are component data products documented and consistent with the FPP model? | 🟢 YES @c14058e29e9e | D8 pass (no failures concern this item) |
| 2.12 | Are all data product containers assigned a default priority? | ⚪ N/A @c14058e29e9e | D4: no data product containers declared |
| 2.13 | Does each component port in the FPP model have a known/planned use-case? | 🟠 NO @c14058e29e9e | R6: port 'controlIn' has no requirement referencing it (add a requirement mentioning 'controlIn')<br>R6: port 'pingIn' has no requirement referencing it (add a requirement mentioning 'pingIn')<br>R6: port 'Run' has no requirement referencing it (add a requirement mentioning 'Run')<br>R6: port 'configureSectionGroupRate' has no requirement referencing it (add a requirement mentioning 'configureSectionGroupRate')<br>R6: port 'packetOverrideReturnIn' has no requirement referencing it (add a requirement mentioning 'packetOverrideReturnIn')<br>R6: port 'TlmRecv' has no requirement referencing it (add a requirement mentioning 'TlmRecv')<br>... 5 more |
| 2.14 | Are port directions and types as expected? | 🟠 NO @c14058e29e9e | D7: port 'PktSend' (output output Fw.Com) is missing from the SDD port table<br>D7: port 'Run' (async input Svc.Sched) is missing from the SDD port table<br>D7: port 'TlmGet' (sync input Fw.TlmGet) is missing from the SDD port table<br>D7: port 'TlmRecv' (sync input Fw.Tlm) is missing from the SDD port table<br>D7: port 'configureSectionGroupRate' (async input ConfigureGroupRate) is missing from the SDD port table<br>D7: port 'controlIn' (async input EnableSection) is missing from the SDD port table<br>... 15 more |
| 2.15 | Is the component type as expected (passive, queued or active)? | 🟠 NO @c14058e29e9e | D9: SDD does not state that TlmPacketizer is an active component |
| 2.16 | Are all the component requirements mapped to design description and/or artifacts? | 🟠 NO @c14058e29e9e | D5: requirement 'TPK-009' (Svc/TlmPacketizer/docs/sdd.md:27) is not mapped to any design artifact (add a Design column entry or reference it in a design section)<br>D5: requirement 'TPK-010' (Svc/TlmPacketizer/docs/sdd.md:28) is not mapped to any design artifact (add a Design column entry or reference it in a design section)<br>D5: requirement 'TPK-011' (Svc/TlmPacketizer/docs/sdd.md:29) is not mapped to any design artifact (add a Design column entry or reference it in a design section) |
| 2.17 | Does the component FPP compile and generate auto-code? | 🟠 NO @c14058e29e9e | D6: fpp-check failed: fpp-check Svc/TlmPacketizer/config/TlmPacketizerConfig/TlmPacketizerCfg.fpp:8.1 type FwTlmPacketizeOffsetType = U16 ^ error: redefinition of symbol FwTlmPacketizeOffsetType previous definition is here: build-fprime-automatic-native/F-Prime/Svc/TlmPacketizer/config/TlmPacketizerConfig/TlmPacketizerCfg.fpp:8.1 type FwTlmPacketizeOffsetType = U16 ^ |

## 3. Implementation

| # | Item | Status | Notes |
|---|------|--------|-------|
| 3.1 | Are all component source dependencies listed in the CMakeLists.txt file for the component? | 🟠 NO @c14058e29e9e | I1: dependency 'Fw' (from #include "Fw/FPrimeBasicTypes.hpp") is not declared in CMakeLists.txt (add it to DEPENDS as Fw)<br>I1: dependency 'Fw/Buffer' (from #include "Fw/Buffer/Buffer.hpp") is not declared in CMakeLists.txt (add it to DEPENDS as Fw_Buffer)<br>I1: dependency 'Fw/Com' (from #include "Fw/Com/ComPacket.hpp") is not declared in CMakeLists.txt (add it to DEPENDS as Fw_Com)<br>I1: dependency 'Fw/DataStructures' (from #include "Fw/DataStructures/Array.hpp") is not declared in CMakeLists.txt (add it to DEPENDS as Fw_DataStructures)<br>I1: dependency 'Fw/Prm' (from #include "Fw/Prm/ParamValid.hpp") is not declared in CMakeLists.txt (add it to DEPENDS as Fw_Prm)<br>I1: dependency 'Os' (from #include "Os/Mutex.hpp") is not declared in CMakeLists.txt (add it to DEPENDS as Os) |
| 3.2 | Does the component compile without warnings for all required targets? | 🟢 YES @c14058e29e9e | I2 pass (1 log(s) scanned) |
| 3.3 | Are all loops properly terminated/bounded? | 🟠 NO @c14058e29e9e | `cpp/fprime/jpl-c/loop-bounds` Svc/TlmPacketizer/TlmPacketizer.cpp:250 This loop does not have a fixed bound. |
| 3.4 | Has gating been provided for any potential cyclic error/status messages (e.g. log messages, events)? | 🟢 YES @c14058e29e9e | `cpp/fprime/unthrottled-cyclic-event` 0 findings |
| 3.5 | Do all switch statements have a break after each case and terminate with a default case? | 🟢 YES @c14058e29e9e | `cpp/fprime/switch-default` 0 findings<br>`cpp/fprime/switch-case-fallthrough` 0 findings |
| 3.6 | Verify all non-empty case statements that fall through to the following case statement are explicitly commented as such | 🟢 YES @c14058e29e9e | `cpp/fprime/switch-case-fallthrough` 0 findings |
| 3.7 | Are all variables locally defined unless global or specific visibility is required? | 🟢 YES @c14058e29e9e | `cpp/fprime/global-mutable-variable` 0 findings |
| 3.8 | Are all class members defined appropriately using private, public and protected keywords? | 🟢 YES @c14058e29e9e | `cpp/fprime/public-data-member` 0 findings |
| 3.9 | Verify that no floating point expressions are tested for equality/inequality | 🟢 YES @c14058e29e9e | `cpp/fprime/float-equality` 0 findings |
| 3.10 | Is the precision of floating point numbers sufficient to ensure accuracy and floating-point equivalence is evaluated with margin (i.e. safe floating-point usage), and is not used in a loop control? | 🟢 YES @c14058e29e9e | `cpp/fprime/float-narrowing` 0 findings<br>`cpp/fprime/float-loop-control` 0 findings<br>`cpp/fprime/float-equality` 0 findings |
| 3.11 | Are all variables initialized before use? | 🟢 YES @c14058e29e9e | `cpp/fprime/uninitialized-member` 0 findings<br>`cpp/uninitialized-local` 0 findings |
| 3.12 | Are the priorities of all data products produced by the component configurable via command(s) or parameter(s)? | ⚪ N/A @c14058e29e9e | I3: no data product containers declared |
| 3.13 | Are all commands acknowledged with success/failure? | 🟢 YES @c14058e29e9e | `cpp/fprime/command-response` 0 findings |
| 3.14 | Do all validation failure cases for port and command handlers include a return statement to avoid unintended code execution beyond validation failures? | 🟢 YES @c14058e29e9e | `cpp/fprime/validation-failure-return` 0 findings |
| 3.15 | Are all internal port interfaces invoked via the auto-coded internal port invoke() function? | 🟢 YES @c14058e29e9e | `cpp/fprime/internal-port-invoke` 0 findings |
| 3.16 | All pointer arguments to functions are checked to be not nullptr using FW_ASSERT | 🟠 NO @c14058e29e9e | `cpp/fprime/checking-parameter-values` Svc/TlmPacketizer/TlmPacketizer.cpp:653 Parameter pkt is used as an array index or divisor without first being checked (e.g. with FW_ASSERT or a condition).<br>`cpp/fprime/checking-parameter-values` Svc/TlmPacketizer/TlmPacketizer.cpp:835 Parameter pkt is used as an array index or divisor without first being checked (e.g. with FW_ASSERT or a condition). |
| 3.17 | Assumptions on software state are validated using FW_ASSERT. | 🟠 NO @c14058e29e9e | `cpp/fprime/jpl-c/use-of-assertions-density` Svc/TlmPacketizer/TlmPacketizer.cpp:492 All functions of more than 10 lines should have at least one assertion, unless they handle errors by returning a status enum or bool, or by emitting a WARNING/FATAL event.<br>`cpp/fprime/jpl-c/use-of-assertions-density` Svc/TlmPacketizer/TlmPacketizer.cpp:562 All functions of more than 10 lines should have at least one assertion, unless they handle errors by returning a status enum or bool, or by emitting a WARNING/FATAL event.<br>`cpp/fprime/jpl-c/use-of-assertions-density` Svc/TlmPacketizer/TlmPacketizer.cpp:587 All functions of more than 10 lines should have at least one assertion, unless they handle errors by returning a status enum or bool, or by emitting a WARNING/FATAL event.<br>`cpp/fprime/jpl-c/use-of-assertions-density` Svc/TlmPacketizer/TlmPacketizer.cpp:650 All functions of more than 10 lines should have at least one assertion, unless they handle errors by returning a status enum or bool, or by emitting a WARNING/FATAL event. |
| 3.18 | Assertions shall not be used to validate state that could be dependent on hardware or user input. | 🟠 NO @c14058e29e9e | `cpp/fprime/assert-on-untrusted-input` Svc/TlmPacketizer/TlmPacketizer.cpp:518 FW_ASSERT checks command argument section, which originates from the ground; validate and report instead.<br>`cpp/fprime/assert-on-untrusted-input` Svc/TlmPacketizer/TlmPacketizer.cpp:519 FW_ASSERT checks command argument enable, which originates from the ground; validate and report instead.<br>`cpp/fprime/assert-on-untrusted-input` Svc/TlmPacketizer/TlmPacketizer.cpp:534 FW_ASSERT checks command argument section, which originates from the ground; validate and report instead.<br>`cpp/fprime/assert-on-untrusted-input` Svc/TlmPacketizer/TlmPacketizer.cpp:552 FW_ASSERT checks command argument section, which originates from the ground; validate and report instead.<br>`cpp/fprime/assert-on-untrusted-input` Svc/TlmPacketizer/TlmPacketizer.cpp:553 FW_ASSERT checks command argument rateLogic, which originates from the ground; validate and report instead. |
| 3.19 | Verify all ISR code is free of floating point operations | 🟢 YES @c14058e29e9e | `cpp/fprime/no-float-in-isr` 0 findings |
| 3.20 | Has the ISR code been optimized to minimize length/execution time? | 🟢 YES @c14058e29e9e | `cpp/fprime/isr-complexity` 0 findings |
| 3.21 | Verify any software state that's expected upon receive of an interrupt is configured prior to enabling/setting conditions that would allow the interrupt to trigger. | ⚪ N/A @c14058e29e9e by review | No ISR or interrupt-named function in TlmPacketizer.cpp/.hpp. |
| 3.22 | Verify there are no blocking operations in ISRs | 🟢 YES @c14058e29e9e | `cpp/fprime/no-blocking-in-isr` 0 findings |
| 3.23 | Verify any relevant interrupt enables and interrupts are cleared during initialization and re-enabled selectively as needed. | ⚪ N/A @c14058e29e9e by review | Component enables no interrupt. |
| 3.24 | Are RateGroup schedule calls non-blocking? | 🟢 YES @c14058e29e9e | `cpp/fprime/no-blocking-in-sched-handler` 0 findings |
| 3.25 | Verify no synchronous file access is done within a critical region (e.g. SchedIn) | 🟢 YES @c14058e29e9e | `cpp/fprime/no-file-access-in-critical` 0 findings |
| 3.26 | Verify that all loops and if/else statements are compound statements (i.e. always use brackets {}) | 🟢 YES @c14058e29e9e | `cpp/fprime/compound-statements` 0 findings |
| 3.27 | Verify that all divisions first check for small numbers (including zero) | 🟢 YES @c14058e29e9e | `cpp/fprime/division-guard` 0 findings<br>`cpp/fprime/float-division-guard` 0 findings |
| 3.28 | Verify all array accesses are checked to be within array index bounds | 🟠 NO @c14058e29e9e | `cpp/fprime/unchecked-array-index` 0 findings<br>`cpp/fprime/checking-parameter-values` Svc/TlmPacketizer/TlmPacketizer.cpp:653 Parameter pkt is used as an array index or divisor without first being checked (e.g. with FW_ASSERT or a condition).<br>`cpp/fprime/checking-parameter-values` Svc/TlmPacketizer/TlmPacketizer.cpp:835 Parameter pkt is used as an array index or divisor without first being checked (e.g. with FW_ASSERT or a condition). |
| 3.29 | Verify there are no function declarations within a function. | 🟢 YES @c14058e29e9e | `cpp/fprime/no-function-decl-in-function` 0 findings |
| 3.30 | Verify that the three expressions in all for statements only concern flow control | 🟢 YES @c14058e29e9e | `cpp/fprime/for-header-flow-control` 0 findings |
| 3.31 | Verify that there are no dependencies on signed integer overflow behavior | 🟢 YES @c14058e29e9e | `cpp/fprime/signed-overflow` 0 findings |
| 3.32 | Verify return values are used or voided otherwise. | 🟢 YES @c14058e29e9e | `cpp/fprime/jpl-c/checking-return-values` 0 findings |
| 3.33 | Verify units are incorporated into variable name, where applicable | 🟢 YES @c14058e29e9e by review |  |
| 3.34 | Verify that all invariant numbers are in enums, static constexpr or #defines. Enums or static constexpr must be used unless #define is only way to specify constant. | 🟢 YES @c14058e29e9e | `cpp/fprime/magic-numbers` 0 findings<br>`cpp/fprime/define-constant` 0 findings |
| 3.35 | Verify no use of printf | 🟢 YES @c14058e29e9e | `cpp/fprime/no-printf` 0 findings |
| 3.36 | Does the component use platform independent standard data types (e.g. I32, F32 etc.) rather than primitive data types (e.g. int, float etc.)? | 🟢 YES @c14058e29e9e | `cpp/fprime/jpl-c/basic-int-types` 0 findings |
| 3.37 | Does the component pass static analysis? | 🟠 NO @c14058e29e9e | I4: Svc/TlmPacketizer/TlmPacketizer.cpp:518: [error] cpp/fprime/assert-on-untrusted-input: FW_ASSERT checks command argument section, which originates from the ground; validate and report instead.<br>I4: Svc/TlmPacketizer/TlmPacketizer.cpp:519: [error] cpp/fprime/assert-on-untrusted-input: FW_ASSERT checks command argument enable, which originates from the ground; validate and report instead.<br>I4: Svc/TlmPacketizer/TlmPacketizer.cpp:534: [error] cpp/fprime/assert-on-untrusted-input: FW_ASSERT checks command argument section, which originates from the ground; validate and report instead.<br>I4: Svc/TlmPacketizer/TlmPacketizer.cpp:552: [error] cpp/fprime/assert-on-untrusted-input: FW_ASSERT checks command argument section, which originates from the ground; validate and report instead.<br>I4: Svc/TlmPacketizer/TlmPacketizer.cpp:553: [error] cpp/fprime/assert-on-untrusted-input: FW_ASSERT checks command argument rateLogic, which originates from the ground; validate and report instead.<br>I4: Svc/TlmPacketizer/TlmPacketizer.cpp:653: [medium] cpp/fprime/checking-parameter-values: Parameter pkt is used as an array index or divisor without first being checked (e.g. with FW_ASSERT or a condition).<br>... 2 more |
| 3.38 | Does the implementation address all component requirements and remains consistent with the documented design? | 🟢 YES @c14058e29e9e by review |  |

## 4. Unit Testing

| # | Item | Status | Notes |
|---|------|--------|-------|
| 4.1 | Have all test output/results been published? | ⚫ UNCHECKED @c14058e29e9e | U1 not run: not done yet: no published unit-test artifacts given (--artifacts-dir) |
| 4.2 | Are all component unit test source dependencies listed in the CMakeLists.txt file for the component? | 🟠 NO @c14058e29e9e | U3: unit-test dependency 'Fw/Com' (from #include "Fw/Com/ComPacket.hpp") is not declared in the module's CMake files (add it to UT_DEPENDS/DEPENDS) |
| 4.3 | Have input data ranges been tested for boundary conditions? | 🟢 YES @c14058e29e9e by review |  |
| 4.4 | Do the unit tests pass a memory leak check? | ⚫ UNCHECKED @c14058e29e9e | U4 not run: not done yet: the leak check (unit-test executable under LeakSanitizer/valgrind) has not been run |
| 4.5 | Is there sufficient code coverage? | 🟢 YES @c14058e29e9e | U5 pass (line coverage 95.19% (threshold 80.00%)) |

## 5. Close-Out

| # | Item | Status | Notes |
|---|------|--------|-------|
| 5.1 | Verify no TBDs, TODOs or printfs in the code (no incomplete code). | 🟢 YES @c14058e29e9e | `cpp/fprime/incomplete-code` 0 findings<br>`cpp/fprime/no-printf` 0 findings |
| 5.2 | Verify that component can be instantiated, connected, and compiled along with the Topology for all applicable targets. | ⚫ UNCHECKED @c14058e29e9e | C1 not run: not done yet: no topology build performed by this run (pass --deployment with --build, or the process builds the deployment) |
| 5.3 | Verify that the flight deployment with component instantiation/connections runs successfully on a hardware platform. Indicate hardware platform used for checkout in Notes. | 🟠 NO @c14058e29e9e | Human: Run the flight deployment that instantiates this component on the target hardware and record platform, deployment, commit, date, result and tester in docs/hardware-run.md (check C2 validates the record).<br>C2: no docs/hardware-run.md: record the hardware run (Platform, Deployment, Commit, Date, Result, Tester) |
| 5.4 | Update the SDD to accurately reflect the final state of the component. Add any additional design detail, including any known idiosyncrasies. | 🟠 NO @c14058e29e9e | D7: port 'PktSend' (output output Fw.Com) is missing from the SDD port table<br>D7: port 'Run' (async input Svc.Sched) is missing from the SDD port table<br>D7: port 'TlmGet' (sync input Fw.TlmGet) is missing from the SDD port table<br>D7: port 'TlmRecv' (sync input Fw.Tlm) is missing from the SDD port table<br>D7: port 'configureSectionGroupRate' (async input ConfigureGroupRate) is missing from the SDD port table<br>D7: port 'controlIn' (async input EnableSection) is missing from the SDD port table<br>... 23 more |
| 5.5 | Validate that CI passes. | ⚫ UNCHECKED @c14058e29e9e | C3 not run: not done yet: CI status not consulted (needs the repository, the commit and GITHUB_TOKEN) |

## Hand-off

Nothing to review at the **implementation** stage.
