# Pitfalls

Every entry below was hit for real while building and validating this skill, or
comes from reading production menu code. Read before the first build.

## Blur is not drawn by ImGui

The frosted glass behind a menu is a DWM composition attribute on the host
window, applied via the undocumented `SetWindowCompositionAttribute` with
`ACCENT_ENABLE_BLURBEHIND` (`ui::ApplyWindowBlur`). There is **no** draw-list
way to reproduce it: the draw list cannot read what is behind the window.

Attempts that all fail: layering translucent rects (only dims), sampling the
backbuffer (contains only your own output), a blur shader (nothing to blur).

It needs three things together, and silently degrades if any is missing:

1. `WS_POPUP` + `WS_EX_LAYERED` and `SetLayeredWindowAttributes(..., LWA_ALPHA)`;
2. `DwmExtendFrameIntoClientArea` with `MARGINS{-1,-1,-1,-1}`;
3. the frame cleared to **alpha 0** — `ClearRenderTargetView` with
   `{0,0,0,0}`. Clearing to opaque black gives a black window, which is the
   usual "blur doesn't work" report.

Translucent surfaces then need `alpha < 255` in the theme; a panel at alpha 255
shows no blur no matter how the window is configured.

## Version drift

ImGui's API moves. Pin a tag, and know which side of these you are on:

| Version | Change |
|---------|--------|
| 1.90 | `BeginChild(name, size, bool border, flags)` → `ImGuiChildFlags` |
| 1.91 | `ImDrawCornerFlags_*` fully removed → `ImDrawFlags_RoundCorners*` |
| 1.92 | Font atlas reworked — dynamic sizing, `PushFont(font, size)` |

The template pins `v1.91.9b`: modern draw flags, pre-font-rework. Code written
against a 2023-era fork will not compile here, and vice versa. A forked ImGui
with a modified `BeginCombo` signature is not upstream-compatible at all — treat
such a project as its own API.

## `imgui_internal.h`

`ImLerp`, `ImSaturate`, `ImFabs`, `ImRect`, `ItemSize`, `ItemAdd`,
`ButtonBehavior`, `RenderTextClipped`, `GetCurrentWindow`, `InputTextEx` all
live in `imgui_internal.h`, not `imgui.h`. Including only `imgui.h` produces
`error C3861: 'ImLerp': identifier not found`.

Define `IMGUI_DEFINE_MATH_OPERATORS` **before** the include to get `ImVec2 +
ImVec2`. Guard it — if the build system also defines it, MSVC emits C4005:

```cpp
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"
```

## Windows PowerShell 5.1 reads `.ps1` as ANSI

A helper script containing em-dashes or box-drawing characters, saved as UTF-8
without a BOM, fails to parse with a cascade of misleading
`MissingEndCurlyBrace` errors pointing at correct code. Keep generated `.ps1`
files ASCII-only, or write them with a BOM. This cost a full debugging cycle
during this skill's own construction.

C++ sources are fine as UTF-8 — the template passes `/utf-8` to MSVC.

## Immediate-mode state

- A function-local `static` in a widget is shared by every instance of that
  widget. Key animation state by `ImGuiID` (`animation.md`).
- `ItemSize` before `ItemAdd`. Reversed, the layout cursor drifts by one widget.
- Never hit-test with `IsMouseHoveringRect` in place of `ButtonBehavior`; it
  ignores popups, clipping and item overlap.
- `GetWindowDrawList()` inside a child draws to that child, clipped to it.
  Backgrounds spanning the whole window need `GetBackgroundDrawList()`.
- Drawing to the background draw list ignores `PushStyleVar(ImGuiStyleVar_Alpha)`
  — a fading page will leave its background fully opaque.

## Layout

- `GetWindowWidth()` minus padding is wrong inside a child with a scrollbar;
  use `GetContentRegionAvail().x`.
- A panel's usable height is `size.y - header_h - padding * 2`. Forget it and
  the last control is clipped, which reads as a missing widget.
- `AddRectFilledMultiColor` ignores rounding. Clip or overdraw the corners.
- `AddText` does not clip. Use `RenderTextClipped` for anything that can
  overflow its box.

## The window walks off the screen

Dragging a borderless menu is done by reading ImGui's window position and
calling `MoveWindow` on the host window by that offset, then resetting the
ImGui position to zero. If ImGui restores a **non-zero position from
`imgui.ini`** at startup, that offset is re-applied every single frame and the
window drifts across the display until it leaves it. The first run looks fine
because no ini exists yet; the second run is broken.

```cpp
ImGui::GetIO().IniFilename = nullptr;   // menus do not want window persistence
```

Symptom when screenshotting: the capture comes back blurred and offset, because
`GetWindowRect` is already stale by the time the frame is grabbed.

## Fonts

- A glyph outside the atlas range renders as `?`. ImGui's default range is
  Latin only, so `…` (U+2026), arrows and box characters are all missing.
  Pass an explicit range: `{ 0x0020, 0x00FF, 0x2026, 0x2026, 0 }`.
- The default **stb rasteriser renders lighter and softer than FreeType**.
  Against a reference built with FreeType the text looks thin even at the
  correct size. Build with `-DUI_USE_FREETYPE=ON`, or raise
  `ImFontConfig::RasterizerMultiply` to ~1.15.
- `AddFontFromFileTTF` returns `nullptr` when the file is missing and ImGui
  silently falls back to the built-in font — check the return, or the clone
  ships with the wrong typeface and nobody notices until it looks "off".
- Font files are loaded at runtime from a path relative to the working
  directory. Launching the exe from a different cwd loses them; the template
  copies `assets/` next to the binary and sets the process working directory in
  the capture script.
- Glyph ranges passed to ImGui must be `static` — it stores the pointer.

## Fidelity ceiling — state these, do not paper over them

From a single still image you cannot recover: the typeface (only its category),
exact icon glyphs, hover and active appearance, animation timing and easing, the
alpha of translucent surfaces, or anything behind a closed dropdown. Name what
was inferred in the final report so the user knows exactly which knobs to turn.
