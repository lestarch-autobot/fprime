# Run F´ as a cFS Application

This guide shows how to build an F´ deployment that runs as an application inside a [core Flight System (cFS)](https://github.com/nasa/cFS) mission. It uses the [`fprime_cfs`](https://github.com/fprime-community/fprime_cfs) library, the example application [`fprime_app`](https://github.com/fprime-community/fprime_app), and the working mission [`fprime_cfs_reference`](https://github.com/fprime-community/fprime_cfs_reference).

*Contents*
1. [Prerequisites](#prerequisites)
2. [Overview](#overview)
3. [Step 1 - Build and Run the Reference Mission](#step-1---build-and-run-the-reference-mission)
4. [Step 2 - Add F´ to a cFS Mission](#step-2---add-f-to-a-cfs-mission)
5. [Step 3 - Lay Out the Application](#step-3---lay-out-the-application)
6. [Step 4 - Assign APIDs and Routes](#step-4---assign-apids-and-routes)
7. [Step 5 - Write the Topology](#step-5---write-the-topology)
8. [Step 6 - Write the cFS Entry Point](#step-6---write-the-cfs-entry-point)
9. [Step 7 - Exchange Raw cFS Messages](#step-7---exchange-raw-cfs-messages)
10. [Step 8 - Connect the F´ GDS](#step-8---connect-the-f-gds)
11. [Step 9 - Run the Integration Tests](#step-9---run-the-integration-tests)
12. [Best Practices](#best-practices)
13. [Additional Resources](#additional-resources)

---

## Prerequisites

Before starting, you should have:

- A working F´ development environment and an understanding of F´ topologies and [subtopologies](../develop/develop-subtopologies.md).
- A basic understanding of cFS: applications, the software bus, message ids, and the mission build (`targets.cmake`, `cfe_es_startup.scr`).
- The build dependencies of cFS and of F´ installed on a Linux host.

---

## Overview

`fprime_cfs` is an [F´ library](../develop/develop-fprime-libraries.md) that is also a cFS library. It lets a complete F´ topology run inside one cFS application and talk to the rest of the mission over the cFS software bus.

| Piece | Role |
|---|---|
| `FPrimeCfs.CfsBridge` | Owns the software bus pipe. Receives subscribed messages and publishes outgoing ones. |
| `FPrimeCfs.CfsRouter` | Verifies command checksums and routes commands by APID and function code, telemetry and files by APID. |
| `FPrimeCfs.CfsCmdFramer`, `FPrimeCfs.CfsTlmFramer` | Add the cFS command and telemetry secondary headers to outgoing messages. |
| `FPrimeCfs.CfsAppBridge` | Heads each outgoing framing chain and sets the APID and function code of each message. |
| `FPrimeCfs.SchAppDriver` | Turns a cFS scheduler message into an F´ rate group cycle. |
| `FPrimeCfs.CfsSystemTime` | Provides cFS time to F´. |
| `FPrimeCfs.EvsMirror` | Mirrors F´ text events to cFS Event Services (EVS). |
| `ComCfs` subtopology | The communication stack: bridge, router, framers, and buffer management. |
| `CfsCore` subtopology | Command dispatch, events, telemetry, health, time, and the scheduler driver. |
| `Os_Task_Cfs` | Creates F´ tasks as cFE ES child tasks. |

An F´ deployment imports `ComCfs` and `CfsCore`, and adds its own rate groups and components.

Data moves as follows:

- **Scheduling**: `sch_lab` publishes a tick message. The bridge receives it, the router sends it to `SchAppDriver`, and `SchAppDriver` cycles the rate groups.
- **Uplink**: F´ commands travel as valid cFS command packets on the F´ command APID. The router checks the checksum and passes the F´ command to the command dispatcher.
- **Downlink**: events and telemetry packets leave as cFS telemetry messages, one APID per F´ packet type.
- **Ground**: the F´ GDS sends commands to `ci_lab` and receives telemetry from `to_lab`. No dedicated ground bridge application is needed.

> [!NOTE]
> Application components never build cFS headers or checksums. They exchange payload bytes and an APID; the `ComCfs` stack does the rest.

---

## Step 1 - Build and Run the Reference Mission

Start from a known-good system. Clone the reference mission and install the Python requirements, which include the F´ tools and the `fprime-cfs` GDS plugins:

```bash
git clone --recurse-submodules https://github.com/fprime-community/fprime_cfs_reference.git
cd fprime_cfs_reference
python3 -m venv fprime-venv
source fprime-venv/bin/activate
pip install -r requirements.txt
```

Build with the cFS mission build:

```bash
make SIMULATION=native prep
make
make install
```

Run cFS:

```bash
cd build-artifacts/exe/cpu1/
./core-cpu1
```

The mission starts these applications from `fprime_cfs_reference_defs/cpu1_cfe_es_startup.scr`:

| Application | Role |
|---|---|
| `sch_lab` | Publishes the 1 Hz tick that drives the F´ rate groups. |
| `fprime_app` | The F´ deployment. |
| `ci_lab` | Receives command packets over UDP port 1234 and publishes them on the software bus. |
| `to_lab` | Forwards subscribed telemetry messages over UDP port 2234. |
| `raw_commander` | A plain cFS application that sends a raw command to `fprime_app` and checks the echo. |
| `raw_commanded` | A plain cFS application that echoes a raw command sent by `fprime_app`. |

The cFS console shows F´ events through EVS once `fprime_app` is running.

---

## Step 2 - Add F´ to a cFS Mission

To add F´ to your own mission, place `fprime` and `fprime_cfs` where the cFS build can find them (the reference uses `libs/`) and your application under `apps/`. Then make four changes in the mission definitions directory.

1\. Add F´, the library, and the application to `targets.cmake`. Include `sch_lab`, `ci_lab`, and `to_lab` if you use them:

```cmake
list(APPEND MISSION_GLOBAL_APPLIST fprime fprime_cfs fprime_app sch_lab ci_lab to_lab)
```

2\. Tell the build where the library is, in `mission_build_custom.cmake`:

```cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../libs/fprime_cfs/cmake")
list(APPEND FPRIME_LIBRARY_LOCATIONS "${CMAKE_CURRENT_LIST_DIR}/../libs/fprime_cfs")
```

Add the `CMAKE_MODULE_PATH` line to `arch_build_custom.cmake` as well.

3\. Start the application in `cpu1_cfe_es_startup.scr`. The entry point name must match the function in [Step 6](#step-6---write-the-cfs-entry-point):

```text
CFE_APP, fprime_app,  FPRIME_APP_Main,    FPRIME_APP,   25,   65536, 0x0, 0;
```

4\. Schedule the tick in the `sch_lab` table (`tables/cpu1_sch_lab_table.c` in the reference). This entry sends the tick once per second:

```c
SCH_LAB_ScheduleTable_t Schedule = {
    .TickRate = 100, /* 100 timer ticks per second */
    .Config   = {
        /* Send the F Prime tick once per second (every 100 ticks) */
        {CFE_SB_MSGID_WRAP_VALUE(FPRIME_TICK_MID), 100, 0},
    }
};
```

---

## Step 3 - Lay Out the Application

The quickest start is to copy `fprime_app` and rename it. Its layout combines a cFS application and an F´ project:

```text
fprime_app/
├─ CMakeLists.txt            # cFS app and F´ project
├─ mission_build.cmake       # cFS mission-scope configuration files
├─ arch_build.cmake          # cFS platform-scope configuration files
├─ config/  fsw/  eds/       # cFS configuration headers, sources, and tables
├─ fprime_config/config/     # F´ configuration overrides (APIDs, routes)
├─ Components/               # project components
└─ FPrimeDeployment/
   ├─ CMakeLists.txt
   ├─ Main.cpp               # cFS entry point
   └─ Top/                   # instances.fpp, topology.fpp, topology C++
```

The top-level `CMakeLists.txt` registers the F´ project and adds the F´ directories:

```cmake
project(CFE_FPRIME_APP C CXX)
register_fprime_project()
add_compile_options(-fPIC)

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/fprime_config/config")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Components")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/FPrimeDeployment")
```

`mission_build.cmake` and `arch_build.cmake` both `include(cfs_compat)`, the CMake module that `fprime_cfs` provides.

`FPrimeDeployment/CMakeLists.txt` builds the cFS application around `Main.cpp`, links the topology, and selects the cFS task implementation:

```cmake
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Top/")

add_cfe_app(fprime_app "${CMAKE_CURRENT_LIST_DIR}/Main.cpp")
target_link_libraries(fprime_app ${FPRIME_CURRENT_MODULE}_Top)
add_dependencies(fprime_app ${FPRIME_CURRENT_MODULE}_Top)
set_target_properties(fprime_app PROPERTIES FPRIME_TYPE Deployment)
fprime_attach_custom_targets(fprime_app)
set(FPRIME_USE_PLAIN_LINK_SIGNATURE ON)
# Create F Prime tasks as cFE ES child tasks so they are valid cFS tasks
fprime_target_implementations(fprime_app Os_Task_Cfs)
```

> [!NOTE]
> The deployment is built with `add_cfe_app`, not `register_fprime_deployment`. The build output is a cFS application loaded by `core-cpu1`, not a standalone executable.

---

## Step 4 - Assign APIDs and Routes

Every message on the software bus is identified by an APID. The project configuration in `fprime_config/config/` overrides two F´ configuration files:

```cmake
register_fprime_config(
    CONFIGURATION_OVERRIDES
        "${CMAKE_CURRENT_LIST_DIR}/ComCfg.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/CfsRouterCfg.fpp"
    INTERFACE
    DEPENDS
        FPrimeCfs_CfsBridge_Types
        FPrimeCfs_CfsRouter
)
```

`ComCfg.fpp` holds the `ComCfg.Apid` enumeration. Keep the F´ packet APIDs and add one entry for each cFS message the application sends or receives. The reference adds:

| `ComCfg.Apid` | Value | Use |
|---|---|---|
| `FW_PACKET_COMMAND` | `0x0000` | F´ commands (uplink) |
| `FW_PACKET_LOG` | `0x0002` | F´ events (downlink) |
| `FW_PACKET_PACKETIZED_TLM` | `0x0020` | F´ telemetry packets (downlink) |
| `CFS_SCH_TICK` | `0x0090` | `sch_lab` tick |
| `FPRIME_ECHO_CMD`, `FPRIME_ECHO_TLM` | `0x00A0`, `0x00A1` | Raw command received and echo sent |
| `RAW_ECHO_CMD`, `RAW_ECHO_TLM` | `0x00A2`, `0x00A3` | Raw command sent and echo received |

The reference mission forms cFS message ids from these APIDs: `0x1800 + APID` for commands (the tick is `0x1890`) and `0x0800 + APID` for telemetry (events are `0x0802`).

`CfsRouterCfg.fpp` holds the route table. Each entry sends one APID, and for commands one function code, to an output port index of the router:

```fpp
constant CFS_ROUTER_ROUTE_TABLE = [
    { apid = ComCfg.Apid.FW_PACKET_COMMAND, functionCode = ComCfg.FprimeCommandFunctionCode, routeType = CfsRouteType.FPRIME_COMMAND, portIndex = 0 },
    { apid = ComCfg.Apid.FW_PACKET_FILE, functionCode = 0, routeType = CfsRouteType.FILE, portIndex = 0 },
    { apid = ComCfg.Apid.CFS_SCH_TICK, functionCode = 0, routeType = CfsRouteType.CFS_COMMAND, portIndex = 0 },
    { apid = ComCfg.Apid.FPRIME_ECHO_CMD, functionCode = ComCfg.EchoFunctionCode, routeType = CfsRouteType.CFS_COMMAND, portIndex = 1 },
    { apid = ComCfg.Apid.RAW_ECHO_TLM, functionCode = 0, routeType = CfsRouteType.CFS_TELEMETRY, portIndex = 0 }
]
```

Set `CFS_ROUTER_ROUTE_TABLE_SIZE` in the same file to the number of entries. The `portIndex` of a `CFS_COMMAND` route is the index of `ComCfs.Subtopology.cfsCommandOut` used in the topology; a `CFS_TELEMETRY` route indexes `cfsTelemetryOut`.

---

## Step 5 - Write the Topology

Import the two subtopologies and route the standard F´ services to `CfsCore`:

```fpp
deployment topology FPrimeDeployment {
    import CfsCore.Subtopology
    import ComCfs.Subtopology

    instance rateGroupDriver
    instance rateGroup1

    command connections instance CfsCore.cmdDisp
    event connections instance CfsCore.events
    telemetry connections instance CfsCore.tlmSend
    text event connections instance CfsCore.evsMirror
    health connections instance CfsCore.$health
    time connections instance CfsCore.cfsTime
```

The text event connection to `CfsCore.evsMirror` is what sends F´ events to cFS EVS.

Drive the rate groups from the scheduler tick. Route 0 of the router carries the tick to `SchAppDriver` inside `CfsCore`, which produces the cycle:

```fpp
    connections Scheduler {
        ComCfs.Subtopology.cfsCommandOut[0] -> CfsCore.Subtopology.cfsCommandIn
        CfsCore.Subtopology.schBufferReturnOut -> ComCfs.Subtopology.bufferReturnIn
        CfsCore.Subtopology.cycleOut -> rateGroupDriver.CycleIn
    }

    connections RateGroups {
        rateGroupDriver.CycleOut[Ports_RateGroups.rateGroup1] -> rateGroup1.CycleIn
        rateGroup1.RateGroupMemberOut[0] -> CfsCore.Subtopology.cmdDispRun
        rateGroup1.RateGroupMemberOut[1] -> CfsCore.Subtopology.tlmSendRun
        rateGroup1.RateGroupMemberOut[2] -> CfsCore.Subtopology.healthRun
        rateGroup1.RateGroupMemberOut[3] -> CfsCore.Subtopology.eventsRun
        rateGroup1.RateGroupMemberOut[4] -> ComCfs.Subtopology.bufferManagerSchedIn
    }
```

Connect commanding, downlink, and time between the two subtopologies:

```fpp
    connections Commanding {
        ComCfs.Subtopology.commandOut[0] -> CfsCore.Subtopology.seqCmdBuff
        CfsCore.Subtopology.seqCmdStatus -> ComCfs.Subtopology.cmdResponseIn
    }

    connections Downlink {
        CfsCore.Subtopology.eventsPktSend -> ComCfs.Subtopology.comIn[Ports_ComPacketQueue.EVENTS]
        CfsCore.Subtopology.tlmSendPktSend[0] -> ComCfs.Subtopology.comIn[Ports_ComPacketQueue.TELEMETRY]
    }

    connections Time {
        ComCfs.Subtopology.cfsTimeConvertOut -> CfsCore.Subtopology.cfsTimeConvert
    }
}
```

`CfsCore.tlmSend` is a telemetry packetizer, so the topology also needs a `telemetry packets` specification. cFS expects one layout per APID, so the reference defines a single packet in `FPrimeDeploymentPackets.fppi` and omits every other channel.

The topology C++ (`FPrimeDeploymentTopology.cpp`) is the same as in any F´ deployment: set the rate group divisors, configure the rate group, and give the packet list to `CfsCore::tlmSend`. `TopologyState` carries the state of each subtopology:

```cpp
struct TopologyState {
    CfsCore::SubtopologyState cfsCore;  //!< Subtopology state for CfsCore
    ComCfs::SubtopologyState comCfs;    //!< Subtopology state for ComCfs
};
```

> [!NOTE]
> F´ task priorities map to cFS priorities, where a lower number is more urgent. Choose rate group priorities with the application priority in `cfe_es_startup.scr` in mind.

---

## Step 6 - Write the cFS Entry Point

`Main.cpp` provides the cFS application entry point instead of `main()`. It initializes the F´ OSAL, sets up the topology, subscribes to the software bus, and then lets the bridge process messages until cFS stops the application:

```cpp
void FPRIME_APP_Main(void) {
    Os::init();

    FPrimeApp::TopologyState& inputs = g_topologyState;
    uint32 run_status = CFE_ES_RunStatus_APP_RUN;

    CFE_Status_t status = FPRIME_APP_Init(inputs);
    if (status != CFE_SUCCESS) {
        CFE_ES_ExitApp(CFE_ES_RunStatus_APP_ERROR);
        return;
    }

    // The rate groups are driven by cFS scheduler (SCH) tick messages received
    // through the bridge and routed to the SchAppDriver; no polling timer is needed
    while (CFE_ES_RunLoop(&run_status) == true)
    {
        ComCfs::cfsBridge.process();
    }
    FPRIME_APP_Shutdown(inputs, run_status);
}
```

The initialization function registers with EVS, calls `FPrimeApp::setupTopology(inputs)`, and then subscribes to each APID the application receives. The `ComCfs` subtopology has already created the software bus pipe during topology setup, so subscriptions come after it:

```cpp
// F Prime commands arrive as cFS command packets (secondary header flag set)
status = ComCfs::cfsBridge.subscribeCfs(ComCfg::Apid::FW_PACKET_COMMAND,
                                        FPrimeCfs::CfsBridge::CfsMessageType::COMMAND);
if (status == CFE_SUCCESS)
{
    status = ComCfs::cfsBridge.subscribeCfs(ComCfg::Apid::CFS_SCH_TICK,
                                            FPrimeCfs::CfsBridge::CfsMessageType::COMMAND);
}
```

Use `CfsMessageType::TELEMETRY` to subscribe to telemetry published by another application. A message needs both a subscription and a route: without the subscription it never reaches the application, and without the route the router has nowhere to send it.

---

## Step 7 - Exchange Raw cFS Messages

A component receives or sends native cFS messages by importing the interfaces in `FPrimeCfs/Interfaces`:

| Interface | Ports | Use |
|---|---|---|
| `FPrimeCfs.CfsCommandHandler` | `cfsCommandIn`, `bufferReturnOut` | Receive routed cFS commands |
| `FPrimeCfs.CfsTelemetryHandler` | `cfsTelemetryIn`, `bufferReturnOut` | Receive routed cFS telemetry |
| `FPrimeCfs.CfsCommandProducer` | `cfsCommandOut`, `cfsCommandReturnIn` | Send cFS commands |
| `FPrimeCfs.CfsTelemetryProducer` | `cfsTelemetryOut`, `cfsTelemetryReturnIn` | Send cFS telemetry |

`EchoResponder` in `fprime_app` receives a command and answers with telemetry:

```fpp
passive component EchoResponder {
    import FPrimeCfs.CfsCommandHandler
    import FPrimeCfs.CfsTelemetryProducer
    ...
}
```

Its module depends on `FPrimeCfs_Interfaces` and `FPrimeCfs_Ports`.

The handler receives the APID, the function code, and the payload. To send, the component fills a buffer with payload bytes and sets the destination APID in the frame context (commands also set the function code):

```cpp
void EchoResponder ::cfsCommandIn_handler(FwIndexType portNum,
                                          const ComCfg::Apid& apid,
                                          U8 functionCode,
                                          Fw::Buffer& data) {
    ...
    Fw::Buffer echo(this->m_payload, static_cast<Fw::Buffer::SizeType>(data.getSize()));
    ComCfg::FrameContext context;
    context.set_apid(this->m_telemetryApid);
    this->cfsTelemetryOut_out(0, echo, context);
    ...
    // The command payload belongs to the router; hand it back once processed
    this->bufferReturnOut_out(0, data);
}
```

Two ownership rules apply:

- A handler must return every received buffer through `bufferReturnOut`.
- A producer owns its buffer again only when it comes back on the return port (`cfsTelemetryReturnIn` or `cfsCommandReturnIn`). Do not reuse it before then.

Wire the component to `ComCfs`. The index of `cfsCommandOut` is the `portIndex` from the route table:

```fpp
connections RawCfsEcho {
    ComCfs.Subtopology.cfsCommandOut[1] -> echoResponder.cfsCommandIn
    echoResponder.bufferReturnOut -> ComCfs.Subtopology.bufferReturnIn
    echoResponder.cfsTelemetryOut -> ComCfs.Subtopology.cfsTelemetryDataIn
    ComCfs.Subtopology.cfsTelemetryDataReturnOut -> echoResponder.cfsTelemetryReturnIn
}
```

A command producer connects to `ComCfs.Subtopology.cfsCommandDataIn` and `cfsCommandDataReturnOut` in the same way, and a telemetry handler to `ComCfs.Subtopology.cfsTelemetryOut`.

Payloads are opaque bytes in the layout the other cFS application defines. They are not F´ serialized values.

---

## Step 8 - Connect the F´ GDS

The `fprime-cfs` Python package in `fprime_cfs/python` registers two [GDS plugins](../operate/develop-gds-plugins.md). The reference `requirements.txt` installs it.

- `cfs-lab` communication plugin: sends commands to `ci_lab` and receives telemetry from `to_lab` over UDP. It also sends the `to_lab` enable-output command.
- `cfs` framing plugin: wraps each F´ command as a cFS command packet and strips the cFS headers from telemetry.

With cFS running, start the GDS from the mission root:

```bash
fprime-gds --communication-selection cfs-lab --framing-selection cfs \
    --dictionary ./build-artifacts/exe/Linux/fprime_app/dict/*Dictionary.json -n
```

`-n` tells the GDS not to start the application, because cFS already runs it. Use `--cfs-address` and `--telemetry-destination` when cFS and the GDS are on different hosts.

`to_lab` forwards only the message ids in its subscription table (`tables/cpu1_to_lab_sub.c` in the reference). Add the F´ downlink message ids there: `0x0801` telemetry, `0x0802` events, `0x0803` files, `0x0805` data products, and `0x0820` packetized telemetry.

> [!WARNING]
> The `ci_lab`/`to_lab` path is plain UDP with no authentication or integrity protection. Use it only on isolated lab and bench networks.

---

## Step 9 - Run the Integration Tests

The reference keeps its [GDS integration tests](../../user-manual/gds/gds-test-api-guide.md) in `test/int`. With cFS and the GDS running:

```bash
cd test/int
pytest --dictionary ../../build-artifacts/exe/Linux/fprime_app/dict/FPrimeDeploymentTopologyDictionary.json \
    --deployment-config int_config.json
```

`test_fprime_cfs_reference.py` checks commands, events, and telemetry through `ci_lab` and `to_lab`. `test_raw_cfs_echo.py` checks the raw echo loops with `raw_commander` and `raw_commanded`.

`int_config.json` maps component types to instance names, because the standard services are inside `CfsCore`:

```json
{
    "Svc.CommandDispatcher": "CfsCore.cmdDisp",
    "Svc.EventManager": "CfsCore.events"
}
```

---

## Best Practices

- Start from `fprime_app` and the reference mission, and change one thing at a time.
- Keep every APID in `ComCfg.Apid`. Pass APIDs to components through configuration, as `echoResponder.configure(ComCfg::Apid::FPRIME_ECHO_TLM)` does.
- Add a subscription in `Main.cpp` and a route in `CfsRouterCfg.fpp` together.
- Keep cFS headers out of components. Exchange payload bytes only.
- Keep F´ commands within the `ci_lab` ingest limit (`CI_LAB_PLATFORM_MAX_INGEST`, 768 bytes by default).

## Additional Resources

- [`fprime_cfs`](https://github.com/fprime-community/fprime_cfs): the library, component SDDs, and GDS plugins.
- [`fprime_app`](https://github.com/fprime-community/fprime_app): the example F´ cFS application.
- [`fprime_cfs_reference`](https://github.com/fprime-community/fprime_cfs_reference): the complete mission and its integration tests.
- [Develop a Subtopology](../develop/develop-subtopologies.md)
- [Implement a Framing Protocol](custom-framing.md)
