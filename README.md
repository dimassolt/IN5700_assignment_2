# Fog & Cloud — Minimal OMNeT++ Simulation

Two modules — **Computer** and **Cloud** — exchange:

1. `1-HELLO` → `2-ACK`
2. `3-TEST` (must be *lost exactly three times*) → `4-ACK` on the 4th attempt

## Build & run (OMNeT++ 6.0.3/6.2.0)
- Open OMNeT++ shell: `opp_shell` or `source <omnetpp-root>/setenv`
- IDE: import project and build; or terminal:
    ```bash
    opp_makemake -f -o fog-sim
    make
    ./fog-sim -u Cmdenv -c General
    ```

## Parameters
- `*.cloud.dropTestAttempts` — how many TEST attempts to drop (default `3`)
- `*.computer.timeout` — retransmission timeout (default `1s`)
- `**.propDelay` — link delay (default `50ms`)

## Garbage collection — no-garbage walkthrough

A second scenario lives under `garbage_collection/`. It models a collector smartphone that
visits two bins from top to bottom, confirms that both are empty, and reports the result to
the cloud. To run it:

```bash
opp_makemake -f --deep -O out -I.
make
./assignment_1 -u Qtenv -c General -f garbage_collection/omnetpp.ini -n .
```

Key configuration knobs live in `garbage_collection/omnetpp.ini` and include the
collector's movement delay (`*.host.moveDuration`), the can response time
(`*.can.responseDelay`, `*.anotherCan.responseDelay`), and the cloud acknowledgement delay
(`*.cloud.ackDelay`).