# HarderAI Enhanced — MK8DX 3.0.5

Enhanced CPU driving and race balance for normal Grand Prix and VS races.

## Features

- Faster CPU mini-turbo charging across all three spark tiers.
- CPU mini-turbos and mushrooms preserve their speed gain under AI slowdown.
- Star speed gains scale with a CPU’s existing pace.
- A 90% pre-AI speed-cap floor during eligible CPU drifts.
- Revised formation spacing, catch-up and catch-down behavior.
- Timed pack compression through AIConfuseTable tuning.
- 100 drift probability across all difficulty/position cells.
- Revised rocket-start, wheelspin and trick probability tables.
- Hold, Counter and Snipe item-decision tuning.
- Per-course handicaps and difficulty-specific pace settings.
- Removal of the ordinary 1.5 CPU ratio ceiling.

The package includes AIRace.bin, Performance.bin, the native module and its matching patch. Human controls remain normal. It runs without companion software.

Experimental drift-completion and snaking components are included. Snaking activation is still under development in this exact release snapshot.

## Installation

Use Mario Kart 8 Deluxe **3.0.5**. Extract the named mod folder into the game’s Mod Data Location, enable it, and restart the title. Use one HarderAI gameplay bundle and one copy of each included parameter file.

Tested on Eden/Linux. Other platforms and network play remain unqualified.

## Open source

[Source, technical reference and releases](https://github.com/gustarmartins/HarderAI-Enhanced)

The C/assembly source rebuilds the published module and patches byte-for-byte with the documented toolchain. SHA-256 checksums and the complete parameter changes are included.
