# Detail checklist

A clone that reads as "close but not it" is almost never wrong about layout. It
is wrong about a dozen small things at once. This list is every detail that was
actually missed on a real clone attempt, in the order they cost the most.

**Method: magnify before judging.** At 1:1 all of these are invisible.

```bash
python scripts/crop_zoom.py original.png clone.png --crop 175,110,470,145 --zoom 4 --out cmp.png
```

Two images stack with a divider, same region, same scale. Work through the
panel one row at a time. Then confirm with pixel probes rather than an opinion:

```bash
python scripts/extract_palette.py original.png --at 424,124 --at 600,101
```

## 1. Knob construction — filled or ring?

The most-missed detail, and the most visible. Two conventions exist:

- **Filled**: solid accent knob, track fills with accent when on (iOS style).
- **Ring**: 1px accent outline over a *dark* fill; the track never takes the
  accent at all.

A ring knob is drawn as two filled rects, not `AddRect`, so the outline stays
crisp at high rounding:

```cpp
dl->AddRectFilled(kmn, kmx, ring_colour, r);
dl->AddRectFilled(kmn + ImVec2(1,1), kmx - ImVec2(1,1), dark_fill, r);
```

Check at 4x: if the knob's centre is darker than its edge, it is a ring.

## 2. Overhang

Knobs are frequently **taller than their track** and hang over it. Measure both
independently — a knob height copied from the track height loses the whole
effect. A slider track of 10px with a 19px knob is normal.

The overhanging part must draw outside the item rect. Do not enlarge the item
to fit it, or the row pitch breaks.

## 3. Track fill

Does the accent fill from the track start to the knob? Look at a slider set to
a mid value, not at zero — at 1% the fill is invisible and you will conclude
there is none. Fill up to the knob's **leading edge**, so nothing peeks out at
zero.

## 4. Surface opacity

Panels are often **not** opaque. A panel body sampling `#151515` over a
`#141414` window is the panel colour at alpha ~40 — nearly transparent, letting
the window blur through — while only the header strip is solid. Filling the
body opaque flattens the design and is easy to miss because both look "dark
grey".

Test: sample the panel body and the window background. A 1–2 unit difference
means a low-alpha overlay, not a solid fill.

## 5. Accent-tinted rules

The line under a panel title is often the accent at ~50% alpha, not the neutral
border colour. Probe it: `#7C3D82` on a `#1A191C` background is 50% of a
`#E663F0` accent, not a grey. Neutral separators *between rows* usually stay
grey — check each one, do not generalise from the first.

## 6. Font size and weight

- **Size**: measure the cap-height band of a known word and scale. A body font
  17px where the original is 20px is a ~20% error that reads instantly as
  "different font", even to someone who cannot name why.
- **Weight**: ImGui's default stb rasteriser renders lighter and softer than
  FreeType, which most real menus use. Either build with FreeType, or raise
  `ImFontConfig::RasterizerMultiply` to ~1.15 as a cheap approximation.

## 7. Glyph coverage

A character outside the atlas range renders as `?`. `…` (U+2026) is outside
ImGui's default Latin range and is exactly the kind of thing that ships broken
because it only appears on truncated text. Extend the range explicitly:

```cpp
static const ImWchar r[] = { 0x0020, 0x00FF, 0x2026, 0x2026, 0 };
```

## 8. Truncation policy

A preview showing three items while five are ticked means the design truncates
by **item count**, not by width. Read it off the image; both policies exist, and
whether the marker is `...` (three glyphs) or `…` (one) is visible at 4x.

## 9. Small geometry

| Detail | How it goes wrong |
|--------|-------------------|
| Colour swatch | square in the original, rectangular in the clone |
| Combo caret | bar count, length, spacing and thickness all differ |
| Label placement | inline-left vs above the box — and the gap between them |
| Row pitch | measure baseline-to-baseline of two consecutive rows |
| Border thickness | 1.0 vs 1.2 is visible on a dark background |
| Corner radius | per element, not one global value |

## 10. Decoration and state

- 1px divider lines that pin a region's width (a line at `x=155` *is* the
  sidebar width).
- Gradient direction, extent and where it fades to zero.
- Scrollbars: if a thin bar sits inside the panel edge, the design has visible
  scrollbars — style them rather than disabling them.
- **State-dependent text colour**: a label that brightens when its toggle is on
  is a deliberate effect, not hover. If one row's label is white and its
  neighbour's is grey in the same screenshot, that is why.

## Sign-off

Before declaring a clone done, magnify these five regions side by side: a
toggle row, a slider at a mid value, a panel header, a combo box, and a
selected sidebar tab. If nothing in the checklist above differs, stop.
