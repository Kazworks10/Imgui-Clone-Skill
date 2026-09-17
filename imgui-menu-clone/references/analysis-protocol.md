# Phase 1 — reading a menu out of an image

Output of this phase is `spec.md`. No C++ is written yet. If you catch yourself
about to type `ImColor(` before the spec exists, stop.

## Step 1 — establish the coordinate system

Everything downstream is in pixels, so the pixel grid must be real.

```bash
python ../scripts/extract_palette.py shot.png --grid 8
```

The header prints `dimensions : W x H`. That is your canvas. Two traps:

- **The screenshot may include the desktop around the menu.** Crop mentally to
  the menu's own bounds and state that offset in the spec, or every coordinate
  will be shifted.
- **The screenshot may be scaled** (a 2x display, or an image resized before
  sharing). If the text looks soft, or a 1px border reads as 2px grey, assume
  scaling and express metrics as ratios of the canvas rather than absolutes.

## Step 2 — decompose the layout

Work outside-in and write coordinates as you go:

1. **Window**: total size, corner radius, whether corners are transparent
   (rounded against the desktop) or filled.
2. **Regions**: sidebar / header / content, and the exact boundary lines. A
   1px line at `x=155` is a fact, not a decoration — it pins the sidebar width.
3. **Panels**: each card/group box — position, size, radius, border, header
   height, title alignment.
4. **Rows**: inside a panel, the vertical rhythm. Measure the gap between two
   consecutive control baselines; that is `ItemSpacing.y`.
5. **Controls**: for each, the label position (left-inline vs above), the
   control's own bounding box, and its alignment (right-aligned, full-width).

Rule of thumb: if you cannot state a number, you have not measured it.
"Roughly rounded" is not a spec entry; `rounding: 5` is.

## Step 3 — extract colours by script, never by eye

```bash
# panel interior only, ignoring the blurred desktop behind the window
python ../scripts/extract_palette.py shot.png --region 165,60,480,395

# exact pixel probes: accent swatch, panel bg, border line, idle text
python ../scripts/extract_palette.py shot.png --at 766,127 --at 400,300 --at 168,200
```

Probe placement matters. Sample:

- **panel background** — dead centre of an empty area of the panel;
- **border** — directly on the 1px line, not one pixel off it;
- **accent** — the most saturated element (swatch, active bar, filled track);
- **idle vs active text** — one glyph stem of each, avoiding antialiased edges;
- **track vs fill** — both ends of a slider.

Two caveats to carry into the spec:

- **Translucent surfaces cannot be resolved from one image.** A sidebar at
  `alpha 170` over a blurred desktop samples differently on every background.
  Record it as "translucent, alpha ≈ N" and let Phase 4 tune it.
- **Antialiased text pixels are blends**, never the real text colour. Sample the
  thickest part of a stem, or take the brightest pixel of the glyph.

## Step 4 — infer the interaction model

The image shows one frozen frame. Everything else is inference and must be
labelled as such in the spec:

- which tab/row is selected, and what marks selection (accent bar, fill, text
  colour, all three);
- which control is hovered, if any;
- what a toggle's off state looks like when only the on state is visible
  (or vice versa);
- what a dropdown looks like open;
- whether animation exists at all.

Ask the user for a short screen recording if animations matter. Absent that,
use the defaults in `animation.md` and say so.

## Step 5 — write `spec.md`

```markdown
# Menu spec — <name>

## Canvas
- size: 800 x 405
- window rounding: 5, corners transparent
- source image: shot.png (807 x 409, menu offset +4,+2)

## Palette            (measured unless marked)
| role            | value             | probe    |
|-----------------|-------------------|----------|
| accent          | ImColor(230,99,240) | 766,127 |
| window bg       | ImColor(20,20,20)   | 600,20  |
| sidebar bg      | ImColor(20,20,20,~170) — translucent, alpha inferred |
| panel bg        | ImColor(26,25,28)   | 400,300 |
| border          | ImColor(33,32,35)   | 480,200 |
| text active     | ImColor(255,255,255)|         |
| text idle       | ImColor(120,120,120)| 190,127 |

## Typography
- body: geometric sans, ~17px  → Poppins Medium (INFERRED)
- logo: heavy display, ~22px, letterspaced → Bungee (INFERRED)
- icons: 4 glyphs — crosshair, sliders, gear, document (INFERRED, see icons.md)

## Regions
- sidebar: x 0..155, translucent, 1px border at x=155
- header:  y 0..55, 1px border at y=55, logo centred in sidebar width
- content: x 165.., two panels of 307 wide, 10px gutter

## Controls
| panel | control | label | metrics |
|-------|---------|-------|---------|
| Recoil | Toggle | left-inline | track 36x18, r=9, knob d=14, right-aligned |
| Recoil | Keybind | left-inline | box 52x24, r=5, text "None" centred |
| Recoil | SliderInt | above + value right | track h=6, full width, knob 22x12 |
...

## Inferred, not measured
- hover states, all animation timing
- exact fonts and icon glyphs
- sidebar alpha
```

## Step 6 — checkpoint

Present the spec and explicitly list the "inferred, not measured" block. That
is the part the user can fix in seconds and you cannot fix at all.
