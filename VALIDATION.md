# Release validation

v0.1.0 freezes the build used during the author's positively received Eden/Linux play session. The live module matched the archived NSO; the resident AIRace image matched the installed release file.

- The public C/assembly build reproduces the module, IPS32 and IPSwitch SHA-256 hashes byte-for-byte.
- 182 focused native guest checks passed for this snapshot: loader behavior, caps/boosts, drift completion, snaking scope/gap rules, and scene/group lifetime handling.
- The guest tests use the original game's instruction fragments and supplied object graphs. The original executable and captured game-memory fixtures are excluded from this public repository. These test counts are local evidence, not a claim that this repository ships those fixtures.
- Public tests cover IPS encoding, source/file hashes and exact parameter reconstruction. They do not emulate an entire race.
- General gameplay feel was accepted by the author. Snaking counters recorded zero starts across the reviewed race segments. Full route clearance, network synchronization and other platforms remain unqualified.

## Known issue preserved in v0.1.0

The snaking controller restarts its waiting timer during ordinary idle native maneuvers. Four later native regression cases reproduce this. The fix is under a separate experimental build and is deliberately excluded from this exact snapshot.

## Future changes

Development fixes and additional enhanced-CPU features will use new commits and release tags. Existing release assets and hashes should remain immutable.
