# Parameters — v0.2.0-rc.1

[Exact tables](parameters.json) · [Every byte edit](parameter_edits.json)

## VLRA — AILevelTable

| Difficulty | Default | Minimum | Formation | CC additions |
|---|---:|---:|---:|---|
| Easy | 0.975 | 0.5 | 1 | 0, 0, 0, 0 |
| Normal | 1 | 0.5 | 1 | 0, 0, 0, 0 |
| Hard | 1.01 | 0.9 | 1 | 0, 0, 0, 0 |
| Expert | 1.02 | 0.75 | 1 | 0, 0, 0, 0 |

## MPRA — AIParamTable

| Field | Value |
|---|---|
| AddSpeedRatio_Up_ByFormation | 0.04 |
| AddSpeedRatio_Down_ByFormation | 0.05 |
| DisMin_CatchUp / DisMax_CatchUp | -900 / 1800 |
| DisMin_CatchDown / DisMax_CatchDown | 1500 / 4000 |

| Difficulty | CatchUp | CatchDown |
|---|---:|---:|
| Easy | 0.1 | -0.1 |
| Normal | 0.1 | -0.035 |
| Hard | 0.135 | -0.025 |
| Expert | 0.115 | -0.015 |

FormationSpan stores increments. Native construction accumulates and scales them into formation targets.

| Slot | Stored increment |
|---|---:|
| 00 | 0 |
| 01 | 411.972 |
| 02 | 480.634 |
| 03 | 549.297 |
| 04 | 617.957 |
| 05 | 686.619 |
| 06 | 823.946 |
| 07 | 961.269 |
| 08 | 1098.59 |
| 09 | 1235.91 |
| 10 | 1373.24 |
| 11 | 1510.56 |

## FCRA — AIConfuseTable

Probability: **90%**. Trigger window: **50–150 racing seconds**.

| Difficulty | Scale minimum | Scale maximum |
|---|---:|---:|
| Easy | 0.75 | 1.25 |
| Normal | 0.35 | 0.75 |
| Hard | 0.2 | 0.5 |
| Expert | 0.2 | 0.75 |

## PIRA — AIItemTable

| Difficulty | Hold | Counter | Snipe |
|---|---:|---:|---:|
| Easy | 100 | 30 | 40 |
| Normal | 100 | 40 | 45 |
| Hard | 100 | 75 | 50 |
| Expert | 100 | 90 | 70 |

## PARA — AIProbability

RocketStart, Drift and JumpAction are **100** across all four difficulty rows and twelve positions. WheelSpin rows:

- **Easy:** 0, 0, 0, 10, 10, 10, 20, 20, 30, 30, 30, 50.
- **Normal:** 0, 0, 0, 0, 0, 0, 10, 10, 20, 20, 30, 30.
- **Hard:** 0, 0, 0, 0, 0, 0, 0, 5, 8, 10, 12, 15.
- **Expert:** 0, 0, 0, 0, 0, 0, 0, 0, 5, 8, 10, 12.

## SCRA — AICourseTable

All 123 × 8 course handicap cells are zero.

## VIRA

The original table is preserved.

## Performance.bin — CPU mini-turbo targets

| Field | Original | Release |
|---|---:|---:|
| TMRP CPU blue | 250 | 100 |
| TMRP CPU orange | 660 | 396 |
| TMRP CPU purple | 1100 | 660 |

CPU row 20, offsets `0xCD0`, `0xCD4`, `0xCD8`. Native charging uses strict threshold comparisons.

The [director](README.md#race-director) uses these authored pace allowances and formation geometry during racing.
