# HarderAI Enhanced

Enhanced CPU driving, item strategy and race dynamics for **Mario Kart 8 Deluxe 3.0.5**.

## Driving

- **Earlier corner drifts.** CPUs prepare their entry and hold suitable corners toward orange and purple turbos.
- **Snaking on straights.** CPUs look ahead for room to charge a blue turbo and adjust their steering through the maneuver.
- **Better drift continuity.** Path joins, changing road slopes, hop landings and banked surfaces receive additional handling checks.
- **Stronger drift finishing.** Short completion windows help CPUs earn the next turbo tier before releasing.
- **Faster charging.** CPU blue, orange and purple targets are **100 / 396 / 660**.
- **Stronger inward steering.** Eligible inward input reaches the native faster charge rate sooner.
- **Useful held-turn charging.** The first blue target also receives a reduced threshold during eligible held turns.
- **Better speed through drifts.** Eligible CPU drifts retain at least 90% of the kart's pre-AI speed cap.
- **Earned boost protection.** Boost-cap calculations preserve the kart's pre-AI cap and an already stronger CPU cap. Star speed scales with the CPU's existing pace.
- **Safer drafting lines.** Racer-following targets are checked against the available path corridor.
- **Full skill probabilities.** Rocket starts, drifts and tricks use 100 across every difficulty and position.

## Items and finishes

- **Rapid Golden Mushrooms.** Repeated use continues through eligible shortcut approaches.
- **Earlier late-race item use.** Golden Mushrooms and Stars gain finish-distance urgency.
- **Shorter Mushroom waits.** CPUs use ordinary Mushrooms more readily while retaining shortcut opportunities.
- **Active-Star shortcuts.** CPUs can select a nearby shortcut while enough Star time remains.
- **First-place defense.** CPUs retain suitable Bananas and Green/Red Shells for protection.
- **Reactive defense.** Super Horn and Star decisions respond to qualified shell and nearby hazard threats.
- **Finish the whole race.** Single-player GP and VS wait for the remaining racers, with a 30-second grace period after the human finishes.

## Race director

Director policies run automatically during single-player GP and VS. They use race progress, actual gaps, measured pace and the included AIRace tune. Timing follows racing updates; at normal speed, 60 updates equal roughly one second.

| Policy | Behavior |
|---|---|
| **Contested opening** | For the first 20 seconds, formation targets use tiers 00–02. The next 10 seconds blend back toward regular targets. |
| **Changing race phases** | Spread, chase and battle phases vary by lap or section, with seeded ordering and timing. |
| **Individual catch pressure** | Each CPU considers the human, the leading CPU and its nearest opponent when calculating catch pressure. |
| **Rotating challengers** | Two nearby trailing CPUs receive attacking opportunities; battle phases can select three. Selection rotates in ten-second cycles. |
| **Pace-aware pursuit** | Clean driving samples estimate ordinary pace. Selected challengers aim toward a 0.3-second gap over a 20-second horizon. Distant recovery uses a 60-second horizon. |
| **Leader containment** | A racer more than two seconds ahead receives separation feedback within the tune's correction allowance. |
| **Smooth compression** | Native confusion depth changes ease toward their new scale over 15 seconds. |
| **Moving ladders** | Every 20 seconds, front, midfield and rear groups rotate through a spacing pulse. Total effective pack depth is preserved while local gaps change. |
| **Graduated formation correction** | Correction strength changes smoothly with each racer's gap and local spacing. |
| **Earned Golden momentum** | A CPU that gains at least two places and 600 progress units with a Golden can earn a temporary front-target opportunity after retaining the gain. |
| **Native slot promotions** | From around 40 seconds, eligible middle/rear CPUs rotate into native slots 00–02 at 20-second opportunities. |
| **Shared leader opportunities** | From around 45 seconds, up to two eligible CPUs temporarily use native SLOT 00 during their AI calculations. Opportunities recur around every 30 seconds and last about four seconds. |
| **Course-aware Lightning** | On the final lap of Dry Dry Desert, Yoshi Circuit, Cheese Land, Riverside Park and Sunset Wilds, positions 9–12 retain a small Lightning chance in compressed packs. Qualifying CPU Lightning waits are shortened. This policy uses Normal Items and a twelve-racer, three-lap grid. |

Native promotions and shared slots wait for suitable driving states. Catch and formation requests remain within the included tune's directional allowances. Current boost states retain the applied pressure while the game's boost physics runs.

## Included balance

The four difficulty rows have separate pace and item settings. HARD starts at **1.010**, with a **0.900** minimum, formation **+0.040 / −0.050**, catch-up **+0.135** over **−900…1800**, and catch-down **−0.025** over **1500…4000**.

[Full parameter tables](PARAMETERS.md) · [Exact byte edits](parameter_edits.json)

## Install

1. Use MK8DX **3.0.5**, build ID `FE941ED5BA14BE5D505698DA1BBF4FE7`.
2. Extract **HarderAI Enhanced v0.2.0-rc.1** into the game's Mod Data Location.
3. Enable that folder as your HarderAI gameplay bundle and restart the title.

The folder includes the native module, matching executable patch, AIRace.bin and Performance.bin. Use one HarderAI executable bundle and one replacement for each parameter file.

**Tested platform:** Eden on Linux. **Release channel:** preview.

## Source

GitHub hosts the source and build recipes. The install package is distributed separately.

- [Native implementation and build commands](TECHNICAL.md)
- [Validation and next improvements](VALIDATION.md)
- [Release files and checksums](release-manifest.json)

Original mod source is MIT-licensed. Game assets and trademarks belong to their respective owners.
