# Native implementation — v0.2.0-rc.1

Target: MK8DX **3.0.5**, title `0100152000022000`, main build ID `FE941ED5BA14BE5D505698DA1BBF4FE7`.

The AArch64 `subsdk9` module is linked at main-relative offset `0x1A49000`. The source profile contains **96 translation units**, **67 module hooks**, exact return branches and source hashes. `main_overlays.json` and the compiled finish companion provide the remaining main-image changes. The combined patch covers **4,912 bytes**.

## Driving

`snaking_v305.c` writes movement-packet requests through the native driving pipeline. Opportunity planning checks selected and BASE route geometry, available width, charge time, path continuity, current road normals and the next turn. Steering compensation and countersteering maintain the planned corridor. Native drift state and physical charge produce the turbo release.

The hop and ground helpers account for current and retained native road queries, the next airborne update, landing interpolation, selected-route changes and adjacent path segments. Qualified predecessor/successor geometry supports continuation across a path joint.

`driving_quality_v305.c` and its corner helpers evaluate longer holds toward orange/purple. The corner geometry check first runs the chassis-frame forecast. A fallback can repeat the full forecast using a matching current native road target when contact, ordinary-ground state, ownership and directional agreement qualify.

`bounded_drift_heading_v305.c` applies bounded heading adjustments to eligible maneuvers. Earlier-entry feedback, inward-input qualification, near-tier completion and directional clearance remain separate stages in the native controller.

## Speed caps and charging

- Ordinary cap ratio: `physics+0x428 / physics+0x420`.
- Boost cap: `physics+0x430`.
- Eligible explicit drift states 4/5 receive a floor of `0.90 × physics+0x424` on the applicable cap fields.
- Earned-boost calculation uses at least `max(native_selected_base, physics+0x424, physics+0x428)` before the native boost multiplier.
- The qualified CPU Star path uses `max(unboosted_ratio, 1) × native_star_multiplier × 1.025`.
- CPU TMRP charge targets are `100 / 396 / 660`; the first held-turn blue target uses a `0.70` scale in its qualified path.
- The ordinary CPU publication ceiling uses positive infinity. Native minimums, course parameters and state-specific consumers remain active.

## Director arithmetic

The director hooks the CPU sum before final native limits. Its basic composition is:

```text
ratio = native_base + formation_term + catch_term
ramp(x, low, high) = clamp((x - low) / (high - low), 0, 1)
```

Authored CU/CD windows bound the corresponding per-racer catch requests. Formation uses authored Up/Down allowances. The pressure estimator learns ordinary progress per racing update, normalized by measured physical cap ratio, from qualified clean samples.

Selected challengers request:

```text
gap = reference_progress - cpu_progress
closing = max(0, gap - 0.3 × 60 × reference_rate) / (20 × 60)
requested_ratio = max(native_base, (reference_rate + closing) / cpu_rate_at_1)
```

Distant recovery uses a 60-second horizon. Component response uses `dt / (54 + dt)`; paused racing clocks preserve state.

Moving ladder increments retain their total effective depth. A selected group's spacing uses a smooth pulse `16u²(1−u)²`; the relative weights are normalized back to the total. Native confusion-scale transitions use `S(u)=3u²−2u³` over 900 racing ticks.

For gap error `e` and a half-strength distance `h` derived from local spacing, the soft formation fraction is `e / (h + abs(e))`. Up or Down scales the applicable sign. This keeps correction strength responsive over large gaps.

The shared-slot hook brackets the native CPU-job submit/wake/join interval with a 384-byte invocation frame. It restores owned slot labels after joined work. Native promotions update qualified subgroup membership and ordering at their manager boundary.

## Items and finishing

Golden repetition, ordinary Mushroom waiting, active-Star shortcut selection, first-place holding and Horn/Star reactions retain their native item-admission paths. Finish-urgency helpers use remaining course progress and the current driving estimate to shorten suitable Golden/Star waits.

The five-course Lightning policy raises a qualifying final-lap CPU's normal-distance curve evaluation to at least the curve value at normalized distance `0.3`. Native item masks, rank groups, cooldowns and RNG still select the item. The eligible initial Lightning wait changes from `240–479` to `60–119` planner updates.

`companions/all_racers_finish_v305.S` replaces the single-player GP/VS completion predicate at `main+0x879C2C`. Every racer keeps its own native finish. The predicate admits completion when everyone finishes or the human-reference grace reaches 30,000 ms. The native maximum-race timeout remains active.

## Build

Reference toolchain: **Clang/LLD 23.1.3**, Python 3 and **pyelftools 0.32**.

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements.txt
python build.py --verify
python package.py --airace /path/to/original/AIRace.bin --performance /path/to/original/Performance.bin
python -m unittest discover -s tests -v
```

`--verify` validates source hashes and reconstructs the exact module and executable patches. The compiled finish companion is included in that comparison.

Rebuild parameter files from matching original game files:

```sh
python patch_parameters.py AIRace.bin /path/to/original/AIRace.bin /path/to/output/AIRace.bin
python patch_parameters.py Performance.bin /path/to/original/Performance.bin /path/to/output/Performance.bin
```

`parameter_edits.json` records original hashes, replacement bytes and output hashes. [PARAMETERS.md](PARAMETERS.md) describes the tables.
