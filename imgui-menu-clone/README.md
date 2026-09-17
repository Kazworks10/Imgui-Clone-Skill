# ImGui Menu Clone — a Claude Code skill

Give Claude a screenshot of an ImGui menu. Get back a buildable C++ project that
looks like it.

No source code needed from the original. The image is the whole specification.

## Why it exists

Asking an AI to "rebuild this menu" from a picture lands at 70% right — which is
the most frustrating possible result: recognisably the design, wrong in every
padding, radius and shade. This skill replaces that single leap with a pipeline
where each phase turns one guess into one measurement.

| Phase | What happens |
|-------|--------------|
| 0 · Project | Bootstrap a buildable Win32/D3D11 ImGui project from nothing |
| 1 · Analyse | Screenshot → measured `spec.md`, colours read by script |
| 2 · Checkpoint | You correct the spec before a line of C++ is written |
| 3 · Generate | Theme, layout and any custom widget the design needs |
| 4 · Compare | Build → screenshot → diff against the original → fix |

## Install

Drop the folder into your skills directory:

```
~/.claude/skills/imgui-menu-clone/      (macOS / Linux)
%USERPROFILE%\.claude\skills\imgui-menu-clone\   (Windows)
```

Restart Claude Code. Then just paste a menu screenshot and ask for it to be
rebuilt — the skill loads itself when the request matches.

## What's inside

```
SKILL.md                     the pipeline Claude follows
references/
  analysis-protocol.md       how a screenshot becomes a measured spec
  widget-cookbook.md         custom widgets ImGui doesn't ship
  animation.md               easing, hover and transition timings
  icons.md                   icon fonts, sizing and alignment
  detail-checklist.md        the last 10% that sells the clone
  pitfalls.md                what goes wrong, and why
assets/template/             a working Win32 + D3D11 ImGui project
  ui/theme.h, widgets.*, anim.*, themes/vision.h
scripts/
  bootstrap_project.ps1      toolchain check, ImGui fetch, first build
  capture_window.ps1         screenshot your build for the diff pass
  extract_palette.py         read exact colours out of the reference image
  crop_zoom.py               zoom on a region to measure it properly
```

## Requirements

- Windows with Visual Studio Build Tools and CMake, for the generated project
- Python 3 for the two measurement scripts (`Pillow`)
- Dear ImGui is fetched by the bootstrap script — nothing to vendor by hand

## License

Shared as-is by Palace Hub, free to use and modify. No warranty.
