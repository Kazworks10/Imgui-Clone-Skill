# Icons

A screenshot cannot tell you which font its glyphs came from. Two paths:

## 1 — the user supplies the font (exact match)

Ask for the `.ttf`/`.otf` plus, ideally, the glyph mapping. Without a mapping,
render a sheet of the codepoint range once and read it, rather than guessing.

Custom menu icon fonts frequently map glyphs onto **plain letters** — `"G"`,
`"H"`, `"I"` — instead of the private-use area. If the reference project passes
single capital letters where an icon appears, that is what is happening, and
those letters are meaningless outside that specific font.

## 2 — no font supplied (closest available match)

Default to **Font Awesome 6 Free** with the `IconsFontAwesome6.h` header from
`juliettef/IconFontCppHeaders`. The bootstrap script fetches both. It wins on
three counts: near-total coverage of UI iconography, named defines
(`ICON_FA_GEAR`) so there is zero glyph-mapping guesswork, and it is the de
facto standard in the ImGui ecosystem.

Alternatives, same header repo, when the design is visibly not FA:
**Lucide** / **Feather** (thin 1.5px strokes, rounded caps — modern minimal UI),
**Material Design Icons** (filled, geometric, Google-flavoured),
**Kenney Game Icons** (chunky, game-HUD styling).

Pick by *stroke weight and fill* first — a thin-stroke design re-drawn in filled
Material icons reads as a different product, even with the right symbols.

Then map by meaning, and say in the final report that glyphs are approximations:

| seen in image | likely define |
|---|---|
| crosshair / reticle | `ICON_FA_CROSSHAIRS` |
| sliders, mixer | `ICON_FA_SLIDERS` |
| cog | `ICON_FA_GEAR` |
| document, page | `ICON_FA_FILE` |
| person, avatar | `ICON_FA_USER` |
| eye | `ICON_FA_EYE` |
| shield | `ICON_FA_SHIELD_HALVED` |
| bolt | `ICON_FA_BOLT` |
| wrench | `ICON_FA_WRENCH` |
| chart | `ICON_FA_CHART_LINE` |
| floppy / save | `ICON_FA_FLOPPY_DISK` |
| power | `ICON_FA_POWER_OFF` |

When unsure between two, search the Font Awesome gallery for the concept rather
than inventing a define — a wrong `ICON_FA_*` is a compile error at best and a
blank box at worst.

## Merging into the atlas

Icons merge *into* the text font so `ICON_FA_GEAR "  Settings"` works as one
string:

```cpp
ui::fonts::text = io.Fonts->AddFontFromFileTTF("assets/fonts/Poppins-Medium.ttf", 17.f, &cfg);

static const ImWchar range[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };   // must be static
ImFontConfig icfg;
icfg.MergeMode = true;
icfg.PixelSnapH = true;
icfg.GlyphMinAdvanceX = 16.f;                                        // monospaced icons
io.Fonts->AddFontFromFileTTF("assets/fonts/" FONT_ICON_FILE_NAME_FAS, 15.f, &icfg, range);
```

Four things break this, all silently producing blank boxes:

- the range array not being `static` (ImGui keeps the pointer);
- merging at a size wildly different from the text font — icons sit off-baseline;
- the `.ttf` missing at runtime — `AddFontFromFileTTF` returns `nullptr` and the
  merge is skipped, so always check the return;
- using the **webfont** `fa-solid-900.ttf` is correct; the desktop OTF variants
  use different codepoints than the header expects.

Render icons at 15–16px when body text is 17px: they read heavier than letters
at the same nominal size.
