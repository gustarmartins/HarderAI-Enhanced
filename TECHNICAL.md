# Native implementation — v0.1.0

Target: MK8DX 3.0.5, title `0100152000022000`, main NSO build ID `FE941ED5BA14BE5D505698DA1BBF4FE7`. AArch64 `subsdk9` is linked at main-relative offset `0x1A49000` and contains no imported symbols. Thirteen four-byte main patches branch into it. `profile.json` specifies every original instruction, branch site, continuation and source hash.

## CPU speed-cap consumers

- `earned_boost_v305.S`: on the ordinary local CPU Star branch, replaces `max(r, k)` with `max(r, 1) × k`, where `r` is the unboosted AI ratio and `k` the native Star multiplier. Native Star eligibility/timing is retained.
- On an admitted boost-cap multiplication, the selected base is raised to `max(native_selected_base, physics+0x424, physics+0x428)` before multiplication. This preserves the pre-AI cap and a stronger existing AI cap. Native event selection and boost durations remain in the engine.
- `drift_pace_floor_v305.c`: for eligible local CPU explicit drift states 4/5, raises `physics+0x428` and `physics+0x42C` to at least `0.90 × physics+0x424`. Higher caps remain. Role, backlink, activity and excluded-state checks gate the operation.
- `ratio_ceiling_v305.S`: replaces the ordinary CPU publication ceiling with positive infinity (`0x7F800000`). The human publication path is unchanged. The native ratio calculation supplies the value.

The ordinary physics cap shown by the lab is `physics+0x428 / physics+0x420`. A boost cap at `physics+0x430` is a separate value.

## AIRace and Performance

AIRace tables: `PARA`/AIProbability, `VLRA`/AILevelTable, `PIRA`/AIItemTable, `FCRA`/AIConfuseTable, `MPRA`/AIParamTable and `SCRA`/AICourseTable. `VIRA` is unchanged.

HARD: SpeedRatio_Default 1.020, SpeedRatio_Min 0.500, FormationRatio 0.995; formation Up 0.085, Down 0.075; full CU correction +0.120; CD correction −0.050. Authored CU distance window −5500…2500; CD 5500…10611.111328125. Native course and FCRA scaling may change the runtime distances. Stored FormationSpan values are increments; native construction produces cumulative targets.

The catch ramp uses `clamp((error − Min) / (Max − Min), 0, 1)` multiplied by the signed correction. CU and CD branch selection remains native. Positive formation Down is multiplied by the negative ahead-of-target error fraction. PIRA controls AI item decisions; item roulette distributions are retained.

Performance `TMRP`, CPU row 20, offsets `0xCD0/0xCD4/0xCD8`: `250/660/1100 → 166.6666717529297/440/733.3333129882812`. These are charge targets, with native strict threshold comparisons. Other Performance bytes are unchanged. All AIRace changes, including 558 SCRA cells, are in [PARAMETERS.md](PARAMETERS.md).

## Experimental native driving components

`drift_completion_grace_v305.c`: bounded near-blue hold at the native route-history exit. Slot groups 00–02, 03–05 and 06–08 receive maximum hold budgets of 6, 4 and 2 eligible updates; slots 09–11 receive none. Earlier native exits, native charging and subsequent direction checks remain.

`snaking_v305.c`: requests two opposite blue-turbo drifts on a validated straight route corridor. The native 0x5C210 admission predicate runs before each leg; native 0x5D468 countersteering operates on private phase storage. The controller writes movement-packet requests. Native physics earns charge, changes drift state and admits the boost.

Snaking base cooldowns: 720/1080/1440 ticks for slot groups 00–02/03–05/06–08; remaining slots use native driving. With `w(d)=clamp((d−300)/1200,0,1)`, a follower uses `round(base×(1−0.35w(ahead)))`; a leader uses `round(base×(1+0.50w(behind)))`. New pairs defer within 120 progress units of a neighbor. Initial wait is 240+17×racer ticks; leg deadline 110, settling period 12. Corridor checks cover route keys, unique successors, direction, width, surface alignment and available length.

This frozen version also restarts the cooldown on idle native exclusions. That behavior can starve opportunities and is retained to match the played release exactly. Its later correction is outside v0.1.0.

`com_lifetime_v305.c`: an 88-byte private ABI-2 record follows native scene and racer-group construction/retirement. Companion tools may use it to reject stale tuning contexts. It changes no player roles and performs no host/network I/O.

## Rebuild and verify

Reference toolchain: **Clang 23.1.1, LLD 23.1.1, Python 3, pyelftools 0.32**. Compilation is serial. No game files are needed to compile the authored module and patch.

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements.txt
python build.py --verify
python -m unittest discover -s tests -v
```

`--verify` checks the release source hashes and requires the rebuilt module/IPS hashes to match v0.1.0. It also emits the IPSwitch representation. The original game executable and research workspace are absent from the repository.

Parameter files can be reproduced from matching original 3.0.5 files:

```sh
python patch_parameters.py AIRace.bin /path/to/original/AIRace.bin /path/to/output/AIRace.bin
python patch_parameters.py Performance.bin /path/to/original/Performance.bin /path/to/output/Performance.bin
```

The tool verifies the original hash, applies the documented byte edits and verifies the result hash. It refuses to overwrite an existing output.

## Audit surface

The download contains game data and an AArch64 game module. Installation is a folder copy. Build and parameter tools run locally and contain no download, telemetry, process-attachment or privilege-escalation code. Reproducible hashes connect the published source to the distributed module.
