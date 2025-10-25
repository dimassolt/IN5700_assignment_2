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