---
name: imgui-menu-clone
description: Rebuild a custom Dear ImGui menu in clean C++ from a screenshot alone, without access to the original source. Analyses the image into a measured spec, then generates a themed, animated custom-widget layer and a buildable Win32/D3D11 project. Use when the user supplies an image of an ImGui-style or game-style menu and wants it reproduced in code, wants custom ImGui widgets written to match a design, or needs an ImGui project bootstrapped from nothing (toolchain check, ImGui fetch, fonts, icon font, first build).
---

# ImGui menu clone

Reproduce a menu from an image. The original source is **never** available — the
image is the entire specification. Everything else is measurement and craft.

## The rule that decides whether this works

**Never go image → C++ in one step. Go image → measured spec → C++.**

Writing code straight from a picture means inventing every padding, radius and
colour. That lands at "70% right", which is the most frustrating possible
result: recognisably the design, wrong everywhere. Each phase below exists to
replace one guess with one measurement.

## Pipeline

| Phase | What happens | Skip when |
|-------|--------------|-----------|
| 0 · Project | Bootstrap a buildable ImGui project | a project already exists |
| 1 · Analyse | Image → `spec.md`, colours read by script | never |
| 2 · Checkpoint | User corrects the spec | user asked for one-shot |
| 3 · Generate | Theme + layout + any missing widgets | never |
| 4 · Compare | Build → screenshot → diff → fix | no toolchain available |

### Phase 0 — project

If the user has no project, or asked for one to be set up:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/bootstrap_project.ps1 `
    -Path <target-dir> -Build
```

It checks git / CMake / MSVC (reporting `winget` commands for anything missing,
and installing only with `-InstallMissing`), copies `assets/template/`, fetches
a pinned Dear ImGui, downloads Poppins + Bungee + Font Awesome 6 with its
`ICON_FA_*` header, then configures and builds. Verified working end to end on
Windows 10 + VS 2026 + CMake 4.3.

If the user already has a project, do not restructure it. Read how it declares
widgets and colours, and follow that. If it is a **forked ImGui** with widgets
patched into `imgui_widgets.cpp`, keep writing in that file's style — do not
"fix" the architecture uninvited.

### Phase 1 — analyse

Follow `references/analysis-protocol.md`. Produce `spec.md`, never code.
Colours come from the script, not from looking:

```bash
python scripts/extract_palette.py shot.png --region <x0,y0,x1,y1> --at <x,y>
```

This recovers exact source values — validated against a real menu where it
returned the accent and panel colours bit-for-bit identical to the originals.

### Phase 2 — checkpoint

Show the spec. Ask the user to correct anything wrong, and specifically flag
what an image cannot tell you (list is in the analysis protocol). Thirty
seconds here saves two rebuild cycles. Skip only if the user explicitly wants
one-shot output.

### Phase 3 — generate

Order: **theme → layout → missing widgets**. For each control in the spec:

```
Does an equivalent widget already exist in the project (or the template)?
├── yes, same shape          → reuse it, adjust the theme only
├── yes, but different shape → copy it into a new function and reshape the drawing
└── no                       → write it, following references/widget-cookbook.md
```

Never reshape a widget by editing the shared one; other screens use it.

### Phase 4 — compare

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/capture_window.ps1 `
    -Exe build\Release\menu.exe -Out render.png
```

**Do not judge the result at 1:1.** Comparing full screenshots finds layout
errors and nothing else, which is why a clone can look right and still feel
wrong. Magnify matching regions of both images:

```bash
python scripts/crop_zoom.py original.png render.png --crop 175,110,470,145 --zoom 4 --out cmp.png
```

Walk `references/detail-checklist.md` over at least five regions: a toggle row,
a slider at a mid value, a panel header, a combo box, a selected tab. Fix the
largest discrepancy first, rebuild, repeat. Two or three iterations is normal.
Stop when nothing in the checklist differs.

## Hard rules

1. **No colour literal outside the theme table.** `IM_COL32(...)` inside a
   widget body is a bug. Cloning a second design must mean writing a second
   theme, not editing widgets.
2. **Window blur is not ImGui.** It is a DWM composition attribute on the host
   window (`ui::ApplyWindowBlur`). Never attempt it with the draw list — see
   `references/pitfalls.md`.
3. **Animation state is keyed by `ImGuiID`.** A function-local `static float`
   is shared by every instance of that widget; two toggles will animate as one.
   Use `ui::GetAnim(id)` — `references/animation.md`.
4. **Never patch `imgui_widgets.cpp`** in a project that does not already do it.
   Custom widgets live in their own translation unit.
5. **Hit-testing goes through `ItemAdd` + `ButtonBehavior`.** Manual
   mouse-in-rect tests break with popups, overlapping windows and clipping.
6. **Pin the ImGui version and say so.** 1.90 changed `BeginChild` flags, 1.92
   reworked the font atlas. Code written for one breaks on the other.
7. **State the fidelity ceiling honestly.** Fonts, icon glyphs and hover/active
   states cannot be recovered from a static image. Say what was inferred.

## Reference files

| File | Read it when |
|------|--------------|
| `references/analysis-protocol.md` | starting Phase 1 — includes the spec template |
| `references/detail-checklist.md` | **Phase 4, every time** — what separates 70% from identical |
| `references/widget-cookbook.md` | a control in the spec has no existing equivalent |
| `references/animation.md` | anything moves, fades or slides |
| `references/icons.md` | the design has icons |
| `references/pitfalls.md` | before the first build, and whenever output looks wrong |

Tools: `extract_palette.py` (exact colours, pixel probes), `crop_zoom.py`
(magnified side-by-side), `capture_window.ps1` (render), `bootstrap_project.ps1`
(project from nothing).

## Template layout

`assets/template/` is a working project, not a sketch — it builds and renders.

```
main.cpp              host window + D3D11 + font loading + the layout
ui/theme.h            the theme table (every colour and metric)
ui/themes/vision.h    worked example derived from a real screenshot
ui/anim.h/.cpp        ImGuiID-keyed animation state with GC
ui/widgets.h/.cpp     Toggle Slider Button InputText Combo MultiCombo
                      Keybind ColorPicker Tab GroupBox Separator ShadowRect
ui/shell_win32.h/.cpp borderless + layered + DWM blur window
CMakeLists.txt        pinned ImGui via FetchContent, D3D11 + dwmapi
```

## What to tell the user at the end

- which parts were measured and which were inferred;
- the icon font chosen and that exact glyphs need their own font file;
- anything in the image that ImGui cannot do natively and how it was faked.
