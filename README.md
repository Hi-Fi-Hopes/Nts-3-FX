# Nts-3-FX

Custom effects for the **Korg Nu:Tekt NTS-3 kaoss pad kit**, built with Korg's [logue SDK](https://github.com/korginc/logue-sdk). Hi-Fi Hopes.

GitHub Actions builds every effect on each push. The `.nts3unit` files appear under **Actions → latest run → Artifacts → nts3-units**, and on the **Releases** page for tagged versions.

## Loading an effect onto the NTS-3

1. Install the **KORG KONTROL Editor** (Windows/Mac) and update the NTS-3 to its latest system firmware.
2. Connect the NTS-3 to the computer over USB and open the editor.
3. Drag the `.nts3unit` file onto the user effects list, then press **SEND**.
4. The effect appears at the end of the NTS-3's effect list.

## Effects

### Spring Tank (`spring-tank/`)

A spring reverb modelled on how real tanks work: each spring is a feedback loop with a dispersive allpass chain (the "drip" chirp), a transit delay, and the dull-top / thin-bottom tone of a spring.

| Control | Parameter | What it does |
|---|---|---|
| Pad X | **DWELL** | How hard the springs are driven: clean → splashy and saturated |
| Pad Y | **MIX** | Dry → wet |
| Depth | **DECAY** | Short tank → long tank |
| Quick tap | **CRASH** | Tap and lift within ~0.2 s to bump the tank |
| Editor | **TONE** | Darker ↔ brighter springs |
| Editor | **DRIP** | Amount of chirp/"boing" |
| Editor | **SPRINGS** | 2 or 3 springs |
| Editor | **CHARACTER** | **SR-202W** (short tank, tight and dark) or **SURF** (long tank, wet and drippy) |
| Editor | **CRASH** | Turn the tap-to-crash on or off |

### Sympathetic Strings (`sympathetic-strings/`)

Twelve tuned strings that ring along with whatever you play, like a sitar's sympathetic strings or a piano with the sustain pedal down. Notes in the chosen chord set them ringing; notes outside it barely touch them (about 34 dB less). Changing chord re-tunes the strings with a short glide, so ringing strings slide into the new chord.

| Control | Parameter | What it does |
|---|---|---|
| Pad X | **CHORD** | 7 dreamy voicings in the key, with hysteresis so a resting finger doesn't flicker between chords (major: I maj9, ii m9, iii m7, IV maj9, V sus, vi m9, I open; minor: i m9, bIII maj7, iv m9, bVI maj9, bVII add9, v m7, i open) |
| Pad Y | **RING** | 0.5 – 30 s |
| Depth | **STRINGS** | Strings level, added on top of the untouched dry sound |
| Editor | **KEY** / **SCALE** | Root note, major or minor |
| Editor | **TONE** | String brightness, 800 Hz – 12 kHz |
| Editor | **DRIFT** | Slow random detune between strings, up to ±12 cents (chorus shimmer) |
| Editor | **STABILITY** | Tape wow, flutter, hiss and darkening |

### Bloom (`bloom/`)

Removes the attack from every note and swells it in, then into a soft, slowly moving reverb. Bells become bowed glass; plucks become pads. An 8 ms look-ahead ducks each new attack before it arrives, and the Sustain stage lifts decaying notes back up so they hold like a bowed note while the swell rises.

| Control | Parameter | What it does |
|---|---|---|
| Pad X | **SWELL** | 20 ms – 4 s |
| Pad Y | **SPACE** | Swelled dry → reverb |
| Depth | **DECAY** | Reverb tail 0.5 – 20 s (room size follows) |
| Editor | **SUSTAIN** | How much decaying notes are lifted (up to +24 dB) |
| Editor | **SENS** | How easily a new note restarts the swell |
| Editor | **CURVE** | SOFT (S-curve), LINEAR, LATE (blooms at the end) |
| Editor | **TONE** | Reverb brightness, 1.5 – 12 kHz |
| Editor | **STABILITY** | Tape wow, flutter, hiss and darkening |

### Memory Fog (`memory-fog/`)

A delay where every repeat is older than the last. Each pass through the loop is darkened, thinned, run at a lower sample rate, saturated and hissed, so the damage compounds: the first echo is nearly clean and the tail dissolves into fog. Moving TIME glides the pitch like a tape machine changing speed.

| Control | Parameter | What it does |
|---|---|---|
| Pad X | **TIME** | 20 ms – 1.3 s |
| Pad Y | **AGE** | How much damage each repeat takes |
| Depth | **FEEDBACK** | Number of repeats; 100% is self-sustaining (held by saturation) |
| Editor | **MIX**, **SPREAD** (ping-pong), **WARBLE** (wow/flutter), **HISS**, **TONE** | |

### Aurora (`aurora/`)

A shimmer reverb whose shimmer follows a chord. Two pitch voices re-pitch the reverb's own output and feed it back in, so the tail blooms into chord tones rather than a plain octave stack.

| Control | Parameter | What it does |
|---|---|---|
| Pad X | **CHORD** | Octave, Fifth, Oct 5th, 2 Octaves, 5th 9th, Major, Minor, Sub Oct |
| Pad Y | **BLOOM** | How much shimmer feeds back |
| Depth | **DECAY** | 1 – 30 s |
| Editor | **MIX**, **TONE**, **MOD** (tail movement), **SIZE**, **STABILITY** | |

### Constellation (`constellation/`)

Tiny pitched-up grains of what you play, scattered across the stereo field like stars and softened in a short reverb. The dry sound passes untouched; the sparkles are added on top.

| Control | Parameter | What it does |
|---|---|---|
| Pad X | **DENSITY** | 0.5 – 40 grains per second |
| Pad Y | **HEIGHT** | Pitch sets: Unison Oct, Octave, Oct 5th, Oct 5th 2Oct, High |
| Depth | **SPARKLE** | Sparkle level |
| Editor | **SIZE** (grain length), **SCATTER** (how far back grains reach), **REVERSE**, **SPACE**, **TONE** | |

### Supermassive (`supermassive/`)

A huge, slowly moving reverb to dance around in space and time, in the spirit of Valhalla's big modulated spaces. An 8-line feedback delay network with size-scaled diffusion runs at half sample rate, so it fits rooms far bigger than Haze in the same memory. Decays run up to 99 s, and the top of the pad freezes the tail.

| Control | Parameter | What it does |
|---|---|---|
| Pad X | **SIZE** | Chamber → supermassive (lines 16 – 330 ms); pre-delay grows with it (0 – 120 ms) |
| Pad Y | **DECAY** | 1 – 99 s; the top edge **freezes** the tail (input muted, holds forever) |
| Depth | **MIX** | Dry → wet |
| Editor | **TONE** | Shelf corner, 800 Hz – 10 kHz: above it the tail dies sooner |
| Editor | **SHIMMER** | Octave-up bloom in the feedback, level-controlled so it can't run away |
| Editor | **MOD** | Tail movement |
| Editor | **WARP** | How fast SIZE glides (20 ms – 5 s); fast glides bend the whole tail's pitch |
| Editor | **STABILITY** | Tape wow, flutter, hiss and darkening |

Built in: **redshift** (longer decays darken the tail) and **gravity** (longer decays slow the modulation).

### Glass Bells (`glass-bells/`)

A bell synth you play on the pad, like the NTS-3's own playable programs. Touch to strike; slide to a new note to strike it. Bells ring on after you lift, into a built-in reverb, and play on top of whatever comes into the NTS-3. Each voice is two-operator FM (the classic DX bell), 8 voices.

| Control | Parameter | What it does |
|---|---|---|
| Pad X | **NOTE** | Two octaves of the scale, so every note fits; shows the note name |
| Pad Y | **TONE** | Soft celesta → glass → sparkle (set when each bell is struck) |
| Depth | **LEVEL** | Bell level on top of the input |
| Editor | **KEY** / **SCALE** | Root (C4 – B4) and Penta Maj, Penta Min, Major, Minor, Lydian, Dorian |
| Editor | **RING** | 0.5 – 30 s |
| Editor | **SPACE** | Reverb amount (more space also lengthens the hall) |
| Editor | **REPEAT** | Mallet roll while you hold, up to 12 Hz; 0 = off |

## Listening without the hardware

`tools/render_<effect>.cc` runs the exact same DSP on a computer and writes WAV files, using bells, pads and test signals from `tools/demo_signals.h`:

```
g++ -O2 -std=c++17 -I core/hfh -I tools -I bloom tools/render_bloom.cc -o render_bloom
./render_bloom .
```

The CI run uploads these as the **demo-audio** artifact.

## Layout

- `<effect>/header.c`: name, parameters and default pad mappings
- `<effect>/effect.h`: maps NTS-3 parameters and touches onto the DSP
- `<effect>/unit.cc`, `config.mk`: Korg's template glue
- `<effect>/*_dsp.h`: the DSP itself, with no SDK dependencies
- `core/hfh/`: the shared HFH core DSP library (see `core/README.md`)
- The Makefile comes from Korg's template at build time.

Developer ID `0x48464850` ('HFHP'). Unit IDs: 1 = Spring Tank, 2 = Sympathetic Strings, 3 = Bloom, 4 = Memory Fog, 5 = Aurora, 6 = Constellation, 7 = Supermassive, 8 = Glass Bells.
