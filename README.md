# HarderAI Enhanced

CPU driving and race-balance mod for **Mario Kart 8 Deluxe 3.0.5**.

## What you get

- **Faster CPU mini-turbo charging.** Blue, orange and purple charge targets are reduced to two-thirds of stock.
- **Stronger CPU boost response.** Earned mini-turbos and mushrooms retain useful acceleration and speed under AI slowdown. Star gains scale above an already fast CPU pace.
- **Better speed through drifts.** Eligible CPU drifts retain at least 90% of the kart’s pre-AI speed cap.
- **Retuned pack behavior.** New formation spacing, catch-up/catch-down windows and timed pack compression.
- **More consistent driving skills.** Drift probability is 100 across every difficulty/position cell. Rocket starts, wheelspin and tricks have revised tables.
- **Revised item decisions.** Hold, Counter and Snipe settings are tuned by difficulty.
- **Revised course handicaps and difficulty pace.** The included AIRace profile supplies the complete tune.
- **Higher CPU pace headroom.** The ordinary 1.5 AI ratio ceiling is removed; the native calculation still determines each racer’s pace.

Normal human controls in GP and VS. The mod runs without a website, terminal, or background application.

### Experimental components included in this snapshot

Bounded near-blue drift completion, formation-slot-based snaking opportunities, and native lifetime observations for companion tuning are included in the source/module. Snaking recorded zero starts in the release trial; its idle waiting timer can repeatedly restart during ordinary cornering. A subsequent fix is being tested separately. Treat snaking and drift-extension effectiveness as experimental.

## Install

1. Use MK8DX **3.0.5**, main build ID `FE941ED5BA14BE5D505698DA1BBF4FE7`.
2. Extract the `HarderAI Enhanced v0.1.0` folder from the release ZIP into Eden’s Mod Data Location for this game.
3. Enable it and restart the title. Disable other HarderAI executable bundles and other replacements for `AIRace.bin` or `Performance.bin`.

The folder contains `exefs/subsdk9`, its matching IPSwitch patch, and both parameter files. Keep them together. The ZIP also provides an equivalent IPS32 file outside the installed mod folder; it requires the same module. Install only one patch representation.

**Tested:** Eden on Linux, normal GP/VS play. Other emulators, Switch hardware, network play and other game versions have no release qualification here.

## Source and technical reference

- [Native hooks, formulas and build instructions](TECHNICAL.md)
- [Every parameter change](PARAMETERS.md) · [exact values](parameters.json) · [byte edits](parameter_edits.json)
- [Release manifest and SHA-256 hashes](release-manifest.json)
- [Known limits and test evidence](VALIDATION.md)

Original mod source is MIT-licensed. Game assets and trademarks belong to their respective owners.
