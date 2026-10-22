# Validation — v0.2.0-rc.1

## Verified

- Standalone source reconstructs the installed Human module and merged executable patch byte-for-byte.
- Parameter edit recipes reconstruct the included AIRace.bin and Performance.bin exactly.
- The latest corner change passed 80 native execution cases across Human and spectator builds, 84 package/phone cases, 17 retained-reader cases and six merged-overlay cases.
- A detailed v79 Human capture matched 35 custom snaking requests to physical blue mini-turbo commits on the same native tick across ten CPUs.
- That capture observed two races reaching all twelve native finishes; one was recorded from its start, and one was joined mid-race.
- Further Human telemetry covers ordinary driving, physical turbo releases, director state and course transitions on Eden/Linux.

## Known limitations and next work

- Snaking remains course-dependent. Some attempts cancel during clearance, input or state transitions.
- Consistent safe exits and high-speed recovery need broader course coverage.
- Traffic-aware approaches and overtaking on Shroom Ridge are planned for the next release.
- Sustained purple charging still trails strong human play on long corners.
- Item defense and shortcut decisions need more outcome-level coverage across item and course states.
- Full GP/eight-race VS completion and a deliberately stuck-racer 30-second fallback still need a dedicated pass on this exact release candidate.
- Broader route optimization and additional moving-surface lanes remain under development.

## Evidence interpretation

Native tests exercise supplied geometry and states. Gameplay telemetry records observed behavior on the installed module. Turbo matches establish a committed boost; safe trajectories and realized speed gain require further trajectory evidence.

Platform verification for this candidate covers Eden on Linux. The release targets the MK8DX 3.0.5 executable identified in [TECHNICAL.md](TECHNICAL.md).
