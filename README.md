# Smart Garbage Collection — OMNeT++ Assignment

This repository bundles the deliverables for the IN5700 Assignment 2. It contains two OMNeT++ models:

* `garbage_collection/` — the smart garbage collection scenario with multiple configurations (main submission)

The garbage collection model follows a smartphone collector as it polls two garbage cans and, depending on configuration, escalates to a cloud backend. Mobility is disabled on purpose; insight comes from message exchanges and the custom visualizer.

## Prerequisites

* OMNeT++ 6.2.0 (the version used for testing)
* A shell configured through `opp_env`/`setenv` so `make` and the OMNeT++ toolchain are on the `PATH`

## Repository layout

| Path | Purpose |
|------|---------|
| `garbage_collection/` | Assignment 2 sources, C++ modules, NED files, and `omnetpp.ini` configurations |
| `garbage_collection/results/` | Sample scalar result files for each configuration |
| `fog/` | Legacy fog/cloud handshake demo |
| `Makefile` | Convenience wrapper that builds the `assignment_2` executable |

## Building the project

From the repository root run:

```bash
make clean && make
```

The provided `Makefile` compiles the shared `assignment_2` executable that both subprojects use. You can equally open the project inside the OMNeT++ IDE (*File → Import → Existing Project into Workspace*) and choose *Project → Build All*.

## Running the garbage collection scenarios

### Recommended: OMNeT++ IDE

1. Launch the OMNeT++ IDE and import this repository as a project if you have not already.
2. In the *Project Explorer*, expand `garbage_collection/` and double-click `omnetpp.ini`.
3. In the editor toolbar, click the green *Run* arrow (or press `Ctrl+F11`). The IDE will prompt for a configuration when necessary.
4. Choose one of the configurations listed below and observe the simulation in Qtenv.

### Optional: Command line

```bash
cd garbage_collection
../assignment_2 -u Qtenv -n .. omnetpp.ini -c <ConfigName>
```

Replace `<ConfigName>` with the desired scenario name.

### Available configurations

| Config name | Description | Expected behaviour |
|-------------|-------------|--------------------|
| `NoGarbageInTheCans` | Baseline scenario with empty cans | Cans reply “NO” through the fog link; no collect requests are sent. |
| `GarbageInTheCansAndSlow` | Cloud-centric solution with slow smartphone ↔ cloud links | Smartphone escalates once per can; acknowledgements return via the slow path. |
| `GarbageInTheCansAndFast` | Fog-centric solution with fast can ↔ cloud links | Cans contact the cloud directly; smartphone only retries queries. |

The custom visualizer prints the selected scenario title, plots dynamic delay figures in the top-right corner, and keeps node-level counters for sent/received/lost messages per command.

## Key parameters

You can tune parameters directly in `garbage_collection/omnetpp.ini`:

* `*.canDelay` — round-trip delay between the smartphone and cans over the local link
* `*.hostToCloudDelay`, `*.cloudToHostDelay` — slow channel pair between smartphone and cloud
* `*.canToCloudDelay`, `*.cloudToCanDelay` — fast channel pair between cans and cloud
* `*.can.hasGarbage`, `*.anotherCan.hasGarbage` — per-can fill state at simulation start
* `*.can.lostQueryCount` — number of initial query attempts each can deliberately drops
* `*.host[0].hostSendsCollect`, `*.can.sendCollectToCloud` — toggles deciding who talks to the cloud

Mobility settings are intentionally absent; visual feedback is derived from static module positions and runtime counters gathered by the C++ modules.
