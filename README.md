# Smart Garbage Collection — OMNeT++ Assignment

This repository contains two small OMNeT++ models that were developed as part of the IN5700 coursework:

* `fog/` — the original “hello/test” handshake between a fog computer and a cloud service
* `garbage_collection/` — the smart garbage collection scenarios delivered for Assignment 2

The second module is the main focus of this submission. It simulates a collector smartphone querying two garbage cans and optionally involving a cloud service depending on the chosen configuration. Mobility is deliberately disabled; every insight is obtained through message exchange and visualization.

## Prerequisites

* OMNeT++ 6.0.3 or newer (tested with 6.0.3 and 6.0.4)
* A shell configured with `setenv`/`opp_shell`

## Building the project

From the repository root:

```bash
opp_makemake -f --deep -O out -I.
make
```

This produces the `assignment_2` executable that can be reused for both subprojects. You can also open the workspace in the OMNeT++ IDE and invoke *Project → Build All*.

## Running the garbage collection scenarios

All configurations live in `garbage_collection/omnetpp.ini`. Use the generated binary and tell it to load the garbage collection network and one of the named configurations:

```bash
./assignment_2 -u Qtenv -n . -f garbage_collection/omnetpp.ini -c <ConfigName>
```

Available configurations:

| Config name | Description | Expected behaviour |
|-------------|-------------|--------------------|
| `NoGarbageInTheCans` | Baseline scenario with empty cans | Cans reply “NO” through the fog path, no collect requests are sent. |
| `GarbageInTheCansAndSlow` | Cloud-centric solution with slow smartphone ↔ cloud links | Smartphone escalates once per can; acknowledgements return via the slow path. |
| `GarbageInTheCansAndFast` | Fog-centric solution with fast can ↔ cloud links | Cans contact the cloud directly; smartphone only retries queries. |

Each scenario renders its title inside the custom visualizer. Delay figures in the top-right corner update dynamically based on the active configuration, and per-node counter text shows sent/received/lost message counts broken down by command.

## Key parameters

Important knobs can be tweaked directly in `omnetpp.ini`:

* `*.canDelay` — round-trip fog delay between smartphone and cans
* `*.hostToCloudDelay` / `*.cloudToHostDelay` — slow channel pair between smartphone and cloud
* `*.canToCloudDelay` / `*.cloudToCanDelay` — fast channel pair between cans and cloud
* `*.can.hasGarbage`, `*.anotherCan.hasGarbage` — initial fill state per can
* `*.can.lostQueryCount` — number of query attempts that each can intentionally drops before answering
* `*.host[0].hostSendsCollect` / `*.can.sendCollectToCloud` — toggles deciding who contacts the cloud

All mobility-related parameters were removed to satisfy the “no movement” requirement. The visualization is driven entirely by static module positions and runtime counters gathered from the C++ modules.

## Legacy “fog” handshake

The `fog/` directory still contains the original handshake demo. Build commands are the same; choose the `fog/omnetpp.ini` file and the `General` configuration if you want to run it.