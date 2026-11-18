# F´ (F Prime) Component Development Checklist

**Component Name:** ClearTextDecryptor  
**Component Owner:** _[Owner Name]_  
**Component Contributors:** _[Contributor Names]_  
**Review Date:** 2026-11-18  
**Checklist Version:** 1.2.0

_Pre-filled by fprime-auto-checklist 0.2.2 for `Svc/Ccsds/ClearTextDecryptor` at `7ec169e2636a`. 🟢 YES 36, 🟠 NO 11, ⚪ N/A 8, 🔵 REVIEW 11, ⚫ UNCHECKED 7._
_Section fingerprints (inputs each section judges): 1=b28b7a40faaa 2=330c28afdf54 3=1342924edbb7 4=d7bfa54dbb1d 5=d8b482e717a2_

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
| 1.1 | Do requirements exist in the component SDD that define the component's expected operational behaviors? | 🔵 REVIEW @7ec169e2636a | CL-6: Requirement coverage (1.1-1.6): read every requirement in the SDD and decide whether the aspect this item names (behavior, input data, output data, memory, hardware I/O, runtime performance) is constrained by at least one of them, or is explicitly declared not applicable with a rationale. Deciding which aspect a shall-statement constrains is a judgement; no column or keyword convention is imposed on the requirements table. R1-R6 (traceability of ports, commands, events, telemetry and parameters to requirements) and R8 (1.7) remain tool-checked. |
| 1.2 | Do requirements exist in the component SDD that define input data expected to be received and handled by the component? | 🔵 REVIEW @7ec169e2636a | CL-6: Requirement coverage (1.1-1.6): read every requirement in the SDD and decide whether the aspect this item names (behavior, input data, output data, memory, hardware I/O, runtime performance) is constrained by at least one of them, or is explicitly declared not applicable with a rationale. Deciding which aspect a shall-statement constrains is a judgement; no column or keyword convention is imposed on the requirements table. R1-R6 (traceability of ports, commands, events, telemetry and parameters to requirements) and R8 (1.7) remain tool-checked. |
| 1.3 | Do requirements exist in the component SDD that define output data expected to be produced by the component? | 🔵 REVIEW @7ec169e2636a | CL-6: Requirement coverage (1.1-1.6): read every requirement in the SDD and decide whether the aspect this item names (behavior, input data, output data, memory, hardware I/O, runtime performance) is constrained by at least one of them, or is explicitly declared not applicable with a rationale. Deciding which aspect a shall-statement constrains is a judgement; no column or keyword convention is imposed on the requirements table. R1-R6 (traceability of ports, commands, events, telemetry and parameters to requirements) and R8 (1.7) remain tool-checked. |
| 1.4 | Do requirements exist in the component SDD for all relevant memory usage constraints for the component? | 🔵 REVIEW @7ec169e2636a | CL-6: Requirement coverage (1.1-1.6): read every requirement in the SDD and decide whether the aspect this item names (behavior, input data, output data, memory, hardware I/O, runtime performance) is constrained by at least one of them, or is explicitly declared not applicable with a rationale. Deciding which aspect a shall-statement constrains is a judgement; no column or keyword convention is imposed on the requirements table. R1-R6 (traceability of ports, commands, events, telemetry and parameters to requirements) and R8 (1.7) remain tool-checked. |
| 1.5 | Do requirements exist in the component SDD for all relevant hardware I/O usage constraints for the component? | 🔵 REVIEW @7ec169e2636a | CL-6: Requirement coverage (1.1-1.6): read every requirement in the SDD and decide whether the aspect this item names (behavior, input data, output data, memory, hardware I/O, runtime performance) is constrained by at least one of them, or is explicitly declared not applicable with a rationale. Deciding which aspect a shall-statement constrains is a judgement; no column or keyword convention is imposed on the requirements table. R1-R6 (traceability of ports, commands, events, telemetry and parameters to requirements) and R8 (1.7) remain tool-checked. |
| 1.6 | Do requirements exist in the component SDD for all relevant runtime performance constraints for the component? | 🔵 REVIEW @7ec169e2636a | CL-6: Requirement coverage (1.1-1.6): read every requirement in the SDD and decide whether the aspect this item names (behavior, input data, output data, memory, hardware I/O, runtime performance) is constrained by at least one of them, or is explicitly declared not applicable with a rationale. Deciding which aspect a shall-statement constrains is a judgement; no column or keyword convention is imposed on the requirements table. R1-R6 (traceability of ports, commands, events, telemetry and parameters to requirements) and R8 (1.7) remain tool-checked. |
| 1.7 | Does every requirement specify a verification method? | 🟠 NO @7ec169e2636a | R8: no requirements found |
| 1.8 | Are there any relevant external interface or functional requirements documented elsewhere such as ICDs/FDDs? If so, please specify in notes. | 🟢 YES @7ec169e2636a | D9 pass (no failures concern this item) |

## 2. Design

| # | Item | Status | Notes |
|---|------|--------|-------|
| 2.1 | Is the high level component functionality documented/described in the SDD? | 🟠 NO @7ec169e2636a | SDD: docs/sdd.md graded bronze; silver or better required (filled intro + sections) |
| 2.2 | Is the component block diagram documented in the SDD and consistent with the FPP model? | 🟠 NO @7ec169e2636a | D9: no block diagram (add an image, a mermaid graph/flowchart, or an ASCII-art block under a design heading) |
| 2.3 | Is the component port table documented in the SDD and consistent with the FPP model? | ⚪ N/A @7ec169e2636a | D7: no typed ports in FPP model |
| 2.4 | Are the component input port behaviors described in the SDD? | ⚪ N/A @7ec169e2636a | D10: no typed input ports in FPP model |
| 2.5 | Are the component input port handlers defined? | ⚪ N/A @7ec169e2636a | D1: no typed input ports declared |
| 2.6 | Are the component command handlers defined? | ⚪ N/A @7ec169e2636a | D2: no commands declared |
| 2.7 | Are the component sequence diagrams documented? | 🟠 NO @7ec169e2636a | D9: no sequence diagram (add a mermaid sequenceDiagram, or state 'Sequence diagrams: N/A' with rationale) |
| 2.8 | Are component state machines and use-cases described in the SDD? | 🟠 NO @7ec169e2636a | D9: no use-case / behavior section heading in the SDD |
| 2.9 | Are the state machine actions defined? | ⚪ N/A @7ec169e2636a | D3: no state machines declared |
| 2.10 | Are component commands, telemetry, events and parameters documented in the SDD (and consistent with the FPP model)? | 🟠 NO @7ec169e2636a | D8: no event table in SDD (1 declared in FPP) |
| 2.11 | Are component data products documented and consistent with the FPP model? | 🟢 YES @7ec169e2636a | D8 pass (no failures concern this item) |
| 2.12 | Are all data product containers assigned a default priority? | ⚪ N/A @7ec169e2636a | D4: no data product containers declared |
| 2.13 | Does each component port in the FPP model have a known/planned use-case? | ⚫ UNCHECKED @7ec169e2636a | R6 not run: no ports declared in the FPP model<br>D10: no typed input ports in FPP model |
| 2.14 | Are port directions and types as expected? | ⚪ N/A @7ec169e2636a | D7: no typed ports in FPP model |
| 2.15 | Is the component type as expected (passive, queued or active)? | 🟠 NO @7ec169e2636a | D9: SDD does not state that ClearTextDecryptor is a passive component |
| 2.16 | Are all the component requirements mapped to design description and/or artifacts? | 🟠 NO @7ec169e2636a | D5: no requirements found |
| 2.17 | Does the component FPP compile and generate auto-code? | 🟢 YES @7ec169e2636a | D6 pass (1 FPP file(s) checked) |

## 3. Implementation

| # | Item | Status | Notes |
|---|------|--------|-------|
| 3.1 | Are all component source dependencies listed in the CMakeLists.txt file for the component? | 🟢 YES @7ec169e2636a | I1 pass (0 dependency module(s) checked) |
| 3.2 | Does the component compile without warnings for all required targets? | ⚫ UNCHECKED @7ec169e2636a | I2 not run: not done yet: no build output to scan (the module was not built by this run) |
| 3.3 | Are all loops properly terminated/bounded? | 🟢 YES @7ec169e2636a | `cpp/fprime/jpl-c/loop-bounds` 0 findings |
| 3.4 | Has gating been provided for any potential cyclic error/status messages (e.g. log messages, events)? | 🟢 YES @7ec169e2636a | `cpp/fprime/unthrottled-cyclic-event` 0 findings |
| 3.5 | Do all switch statements have a break after each case and terminate with a default case? | 🟢 YES @7ec169e2636a | `cpp/fprime/switch-default` 0 findings<br>`cpp/fprime/switch-case-fallthrough` 0 findings |
| 3.6 | Verify all non-empty case statements that fall through to the following case statement are explicitly commented as such | 🟢 YES @7ec169e2636a | `cpp/fprime/switch-case-fallthrough` 0 findings |
| 3.7 | Are all variables locally defined unless global or specific visibility is required? | 🟢 YES @7ec169e2636a | `cpp/fprime/global-mutable-variable` 0 findings |
| 3.8 | Are all class members defined appropriately using private, public and protected keywords? | 🟢 YES @7ec169e2636a | `cpp/fprime/public-data-member` 0 findings |
| 3.9 | Verify that no floating point expressions are tested for equality/inequality | 🟢 YES @7ec169e2636a | `cpp/fprime/float-equality` 0 findings |
| 3.10 | Is the precision of floating point numbers sufficient to ensure accuracy and floating-point equivalence is evaluated with margin (i.e. safe floating-point usage), and is not used in a loop control? | 🟢 YES @7ec169e2636a | `cpp/fprime/float-narrowing` 0 findings<br>`cpp/fprime/float-loop-control` 0 findings<br>`cpp/fprime/float-equality` 0 findings |
| 3.11 | Are all variables initialized before use? | 🟢 YES @7ec169e2636a | `cpp/fprime/uninitialized-member` 0 findings<br>`cpp/uninitialized-local` 0 findings |
| 3.12 | Are the priorities of all data products produced by the component configurable via command(s) or parameter(s)? | ⚪ N/A @7ec169e2636a | I3: no data product containers declared |
| 3.13 | Are all commands acknowledged with success/failure? | 🟢 YES @7ec169e2636a | `cpp/fprime/command-response` 0 findings |
| 3.14 | Do all validation failure cases for port and command handlers include a return statement to avoid unintended code execution beyond validation failures? | 🟢 YES @7ec169e2636a | `cpp/fprime/validation-failure-return` 0 findings |
| 3.15 | Are all internal port interfaces invoked via the auto-coded internal port invoke() function? | 🟢 YES @7ec169e2636a | `cpp/fprime/internal-port-invoke` 0 findings |
| 3.16 | All pointer arguments to functions are checked to be not nullptr using FW_ASSERT | 🟢 YES @7ec169e2636a | `cpp/fprime/checking-parameter-values` 0 findings |
| 3.17 | Assumptions on software state are validated using FW_ASSERT. | 🟢 YES @7ec169e2636a | `cpp/fprime/jpl-c/use-of-assertions-density` 0 findings |
| 3.18 | Assertions shall not be used to validate state that could be dependent on hardware or user input. | 🟢 YES @7ec169e2636a | `cpp/fprime/assert-on-untrusted-input` 0 findings |
| 3.19 | Verify all ISR code is free of floating point operations | 🟢 YES @7ec169e2636a | `cpp/fprime/no-float-in-isr` 0 findings |
| 3.20 | Has the ISR code been optimized to minimize length/execution time? | 🟢 YES @7ec169e2636a | `cpp/fprime/isr-complexity` 0 findings |
| 3.21 | Verify any software state that's expected upon receive of an interrupt is configured prior to enabling/setting conditions that would allow the interrupt to trigger. | 🔵 REVIEW @7ec169e2636a | CL-1: ISR state before enable (3.21): for every ISR-named function (*_isr, *interrupt_handler*, *_irq*) find the code that enables its interrupt; every variable the ISR reads or writes must be assigned on a path that completes before that enable call. |
| 3.22 | Verify there are no blocking operations in ISRs | 🟢 YES @7ec169e2636a | `cpp/fprime/no-blocking-in-isr` 0 findings |
| 3.23 | Verify any relevant interrupt enables and interrupts are cleared during initialization and re-enabled selectively as needed. | 🔵 REVIEW @7ec169e2636a | CL-2: Interrupt enables cleared at init (3.23): in the constructor/init/configure/preamble, interrupt-enable bits must be explicitly cleared or masked before peripheral configuration, not assumed clear from reset. |
| 3.24 | Are RateGroup schedule calls non-blocking? | 🟢 YES @7ec169e2636a | `cpp/fprime/no-blocking-in-sched-handler` 0 findings |
| 3.25 | Verify no synchronous file access is done within a critical region (e.g. SchedIn) | 🟢 YES @7ec169e2636a | `cpp/fprime/no-file-access-in-critical` 0 findings |
| 3.26 | Verify that all loops and if/else statements are compound statements (i.e. always use brackets {}) | 🟢 YES @7ec169e2636a | `cpp/fprime/compound-statements` 0 findings |
| 3.27 | Verify that all divisions first check for small numbers (including zero) | 🟢 YES @7ec169e2636a | `cpp/fprime/division-guard` 0 findings<br>`cpp/fprime/float-division-guard` 0 findings |
| 3.28 | Verify all array accesses are checked to be within array index bounds | 🟢 YES @7ec169e2636a | `cpp/fprime/unchecked-array-index` 0 findings<br>`cpp/fprime/checking-parameter-values` 0 findings |
| 3.29 | Verify there are no function declarations within a function. | 🟢 YES @7ec169e2636a | `cpp/fprime/no-function-decl-in-function` 0 findings |
| 3.30 | Verify that the three expressions in all for statements only concern flow control | 🟢 YES @7ec169e2636a | `cpp/fprime/for-header-flow-control` 0 findings |
| 3.31 | Verify that there are no dependencies on signed integer overflow behavior | 🟢 YES @7ec169e2636a | `cpp/fprime/signed-overflow` 0 findings |
| 3.32 | Verify return values are used or voided otherwise. | 🟢 YES @7ec169e2636a | `cpp/fprime/jpl-c/checking-return-values` 0 findings |
| 3.33 | Verify units are incorporated into variable name, where applicable | 🔵 REVIEW @7ec169e2636a | CL-3: Units in names (3.33): every variable, member, parameter, telemetry channel and command argument carrying a physical quantity must encode its unit in the name (timeoutMs, angleRad, sizeBytes) or in an FPP annotation. |
| 3.34 | Verify that all invariant numbers are in enums, static constexpr or #defines. Enums or static constexpr must be used unless #define is only way to specify constant. | 🟢 YES @7ec169e2636a | `cpp/fprime/magic-numbers` 0 findings<br>`cpp/fprime/define-constant` 0 findings |
| 3.35 | Verify no use of printf | 🟢 YES @7ec169e2636a | `cpp/fprime/no-printf` 0 findings |
| 3.36 | Does the component use platform independent standard data types (e.g. I32, F32 etc.) rather than primitive data types (e.g. int, float etc.)? | 🟠 NO @7ec169e2636a | `cpp/fprime/jpl-c/basic-int-types` Svc/Ccsds/ClearTextDecryptor/ClearTextDecryptor.cpp:25 portNum uses the basic integral type int rather than a typedef with size and signedness.<br>`cpp/fprime/jpl-c/basic-int-types` Svc/Ccsds/ClearTextDecryptor/ClearTextDecryptor.cpp:26 securityAssociationIndex uses the basic integral type int rather than a typedef with size and signedness.<br>`cpp/fprime/jpl-c/basic-int-types` Svc/Ccsds/ClearTextDecryptor/ClearTextDecryptor.cpp:27 data uses the basic integral type int rather than a typedef with size and signedness.<br>`cpp/fprime/jpl-c/basic-int-types` Svc/Ccsds/ClearTextDecryptor/ClearTextDecryptor.cpp:28 context uses the basic integral type int rather than a typedef with size and signedness.<br>`cpp/fprime/jpl-c/basic-int-types` Svc/Ccsds/ClearTextDecryptor/ClearTextDecryptor.cpp:34 portNum uses the basic integral type int rather than a typedef with size and signedness.<br>`cpp/fprime/jpl-c/basic-int-types` Svc/Ccsds/ClearTextDecryptor/ClearTextDecryptor.cpp:35 data uses the basic integral type int rather than a typedef with size and signedness.<br>... 1 more |
| 3.37 | Does the component pass static analysis? | 🟢 YES @7ec169e2636a | I4 pass (7 finding(s) in module; threshold medium) |
| 3.38 | Does the implementation address all component requirements and remains consistent with the documented design? | 🔵 REVIEW @7ec169e2636a | CL-4: Implementation matches requirements and design (3.38): for each requirement row locate the handler(s) that satisfy it and confirm the behaviour including each error path the SDD describes; report requirements with no implementing code and handler behaviour the SDD does not describe. |

## 4. Unit Testing

| # | Item | Status | Notes |
|---|------|--------|-------|
| 4.1 | Have all test output/results been published? | ⚫ UNCHECKED @7ec169e2636a | U1 not run: not done yet: no published unit-test artifacts given (--artifacts-dir) |
| 4.2 | Are all component unit test source dependencies listed in the CMakeLists.txt file for the component? | 🟢 YES @7ec169e2636a | U3 pass (2 dependency module(s) checked) |
| 4.3 | Have input data ranges been tested for boundary conditions? | 🔵 REVIEW @7ec169e2636a | CL-5: Boundary tests (4.3): for each bounded quantity (array sizes, FW_ASSERT limits, configuration constants, command-argument ranges, queue depths) a unit test must exercise the limit value and one past it. |
| 4.4 | Do the unit tests pass a memory leak check? | ⚫ UNCHECKED @7ec169e2636a | U4 not run: not done yet: the leak check (unit-test executable under LeakSanitizer/valgrind) has not been run |
| 4.5 | Is there sufficient code coverage? | ⚫ UNCHECKED @7ec169e2636a | U5 not run: not done yet: no coverage/summary.txt; 'fprime-util check --coverage' has not been run for the module |

## 5. Close-Out

| # | Item | Status | Notes |
|---|------|--------|-------|
| 5.1 | Verify no TBDs, TODOs or printfs in the code (no incomplete code). | 🟢 YES @7ec169e2636a | `cpp/fprime/incomplete-code` 0 findings<br>`cpp/fprime/no-printf` 0 findings |
| 5.2 | Verify that component can be instantiated, connected, and compiled along with the Topology for all applicable targets. | ⚫ UNCHECKED @7ec169e2636a | C1 not run: not done yet: no topology build performed by this run (pass --deployment with --build, or the process builds the deployment) |
| 5.3 | Verify that the flight deployment with component instantiation/connections runs successfully on a hardware platform. Indicate hardware platform used for checkout in Notes. | 🟠 NO @7ec169e2636a | Human: Run the flight deployment that instantiates this component on the target hardware and record platform, deployment, commit, date, result and tester in docs/hardware-run.md (check C2 validates the record).<br>C2: no docs/hardware-run.md: record the hardware run (Platform, Deployment, Commit, Date, Result, Tester) |
| 5.4 | Update the SDD to accurately reflect the final state of the component. Add any additional design detail, including any known idiosyncrasies. | 🟠 NO @7ec169e2636a | D7: no typed ports in FPP model<br>D8: no event table in SDD (1 declared in FPP)<br>D9: no block diagram (add an image, a mermaid graph/flowchart, or an ASCII-art block under a design heading)<br>D9: no sequence diagram (add a mermaid sequenceDiagram, or state 'Sequence diagrams: N/A' with rationale)<br>D9: no use-case / behavior section heading in the SDD<br>D9: SDD does not state that ClearTextDecryptor is a passive component<br>... 2 more |
| 5.5 | Validate that CI passes. | ⚫ UNCHECKED @7ec169e2636a | C3 not run: not done yet: CI status not consulted (needs the repository, the commit and GITHUB_TOKEN) |

## Hand-off

Items left for a human or LLM reviewer:

- **1.1** — CL-6: Requirement coverage (1.1-1.6): read every requirement in the SDD and decide whether the aspect this item names (behavior, input data, output data, memory, hardware I/O, runtime performance) is constrained by at least one of them, or is explicitly declared not applicable with a rationale. Deciding which aspect a shall-statement constrains is a judgement; no column or keyword convention is imposed on the requirements table. R1-R6 (traceability of ports, commands, events, telemetry and parameters to requirements) and R8 (1.7) remain tool-checked.
- **1.2** — CL-6: Requirement coverage (1.1-1.6): read every requirement in the SDD and decide whether the aspect this item names (behavior, input data, output data, memory, hardware I/O, runtime performance) is constrained by at least one of them, or is explicitly declared not applicable with a rationale. Deciding which aspect a shall-statement constrains is a judgement; no column or keyword convention is imposed on the requirements table. R1-R6 (traceability of ports, commands, events, telemetry and parameters to requirements) and R8 (1.7) remain tool-checked.
- **1.3** — CL-6: Requirement coverage (1.1-1.6): read every requirement in the SDD and decide whether the aspect this item names (behavior, input data, output data, memory, hardware I/O, runtime performance) is constrained by at least one of them, or is explicitly declared not applicable with a rationale. Deciding which aspect a shall-statement constrains is a judgement; no column or keyword convention is imposed on the requirements table. R1-R6 (traceability of ports, commands, events, telemetry and parameters to requirements) and R8 (1.7) remain tool-checked.
- **1.4** — CL-6: Requirement coverage (1.1-1.6): read every requirement in the SDD and decide whether the aspect this item names (behavior, input data, output data, memory, hardware I/O, runtime performance) is constrained by at least one of them, or is explicitly declared not applicable with a rationale. Deciding which aspect a shall-statement constrains is a judgement; no column or keyword convention is imposed on the requirements table. R1-R6 (traceability of ports, commands, events, telemetry and parameters to requirements) and R8 (1.7) remain tool-checked.
- **1.5** — CL-6: Requirement coverage (1.1-1.6): read every requirement in the SDD and decide whether the aspect this item names (behavior, input data, output data, memory, hardware I/O, runtime performance) is constrained by at least one of them, or is explicitly declared not applicable with a rationale. Deciding which aspect a shall-statement constrains is a judgement; no column or keyword convention is imposed on the requirements table. R1-R6 (traceability of ports, commands, events, telemetry and parameters to requirements) and R8 (1.7) remain tool-checked.
- **1.6** — CL-6: Requirement coverage (1.1-1.6): read every requirement in the SDD and decide whether the aspect this item names (behavior, input data, output data, memory, hardware I/O, runtime performance) is constrained by at least one of them, or is explicitly declared not applicable with a rationale. Deciding which aspect a shall-statement constrains is a judgement; no column or keyword convention is imposed on the requirements table. R1-R6 (traceability of ports, commands, events, telemetry and parameters to requirements) and R8 (1.7) remain tool-checked.
- **3.21** — CL-1: ISR state before enable (3.21): for every ISR-named function (*_isr, *interrupt_handler*, *_irq*) find the code that enables its interrupt; every variable the ISR reads or writes must be assigned on a path that completes before that enable call.
- **3.23** — CL-2: Interrupt enables cleared at init (3.23): in the constructor/init/configure/preamble, interrupt-enable bits must be explicitly cleared or masked before peripheral configuration, not assumed clear from reset.
- **3.33** — CL-3: Units in names (3.33): every variable, member, parameter, telemetry channel and command argument carrying a physical quantity must encode its unit in the name (timeoutMs, angleRad, sizeBytes) or in an FPP annotation.
- **3.38** — CL-4: Implementation matches requirements and design (3.38): for each requirement row locate the handler(s) that satisfy it and confirm the behaviour including each error path the SDD describes; report requirements with no implementing code and handler behaviour the SDD does not describe.
- **4.3** — CL-5: Boundary tests (4.3): for each bounded quantity (array sizes, FW_ASSERT limits, configuration constants, command-argument ranges, queue depths) a unit test must exercise the limit value and one past it.
