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
| Pad X | **CHORD** | 7 dreamy voicings in the key (major: I maj9, ii m9, iii m7, IV maj9, V sus, vi m9, I open; minor: i m9, bIII maj7, iv m9, bVI maj9, bVII add9, v m7, i open) |
| Pad Y | **RING** | 0.5 – 30 s |
| Depth | **MIX** | Dry → strings |
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

Developer ID `0x48464850` ('HFHP'). Unit IDs: 1 = Spring Tank, 2 = Sympathetic Strings, 3 = Bloom.
