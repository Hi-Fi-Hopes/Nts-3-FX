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

## Listening without the hardware

`tools/render_demo.cc` runs the exact same DSP on a computer and writes WAV files (clap, FM notes, crash, for both characters):

```
g++ -O2 -std=c++17 -I spring-tank tools/render_demo.cc -o render_demo
./render_demo .
```

The CI run uploads these as the **demo-audio** artifact.

## Layout

- `<effect>/header.c`: name, parameters and default pad mappings
- `<effect>/effect.h`: maps NTS-3 parameters and touches onto the DSP
- `<effect>/unit.cc`, `config.mk`: Korg's template glue
- `<effect>/*_dsp.h`: the DSP itself, with no SDK dependencies
- `core/hfh/`: the shared HFH core DSP library (see `core/README.md`)
- The Makefile comes from Korg's template at build time.

Developer ID `0x48464850` ('HFHP'). Unit IDs: 1 = Spring Tank.
