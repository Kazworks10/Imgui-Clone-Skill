# Animating immediate-mode widgets

An ImGui widget owns no memory across frames, so any animated value must live
in a side table keyed by the widget's `ImGuiID`.

## The pattern

```cpp
const ImGuiID id = window->GetID(label);
bool hovered, held;
const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

ui::AnimState& st = ui::GetAnim(id);
if (!st.initialized) {              // seed to the target, so the widget does
    st.initialized = true;          // not animate in from zero on first frame
    st.f[0] = *v ? 1.f : 0.f;
    st.c[0] = *v ? theme.accent : theme.toggle.track_off;
}
ui::Approach(st.f[0], *v ? 1.f : 0.f, theme.motion.speed);
ui::Approach(st.c[0], hovered ? theme.text.hover : theme.text.idle,
             theme.motion.speed_slow);
```

`Approach` is exponential smoothing with the blend factor clamped:

```cpp
const float a = ImSaturate(ImGui::GetIO().DeltaTime * speed);
v = ImLerp(v, target, a);
```

**The clamp is not cosmetic.** On a frame hitch (window drag, first frame,
breakpoint) `DeltaTime * speed` exceeds 1 and an unclamped `ImLerp` overshoots,
producing a visible snap-back. This is a real bug in most menu codebases.

## Why not a function-local static

```cpp
static float alpha = 0.f;           // WRONG
```

Every instance of the widget shares it. Two toggles in the same panel animate
as one, and a value from a hidden tab bleeds into a visible one. The `ImGuiID`
key is what makes the state per-widget.

Note that `ui::GetAnim` collects entries unseen for 300 frames, so hidden tabs
do not leak memory — a common flaw in the `static std::map` variant.

## Choosing speeds

`speed` is "how many e-foldings per second"; higher is snappier. Calibrated
against real menus:

| Effect | speed | feel |
|--------|-------|------|
| Text colour fade | 6 | soft, trails the pointer slightly |
| Background / border / accent | 8–9 | the default; reads as instant but smooth |
| Knob or fill position | 12–18 | must arrive before the eye looks for it |
| Cross-tab page fade | 15 | ~70ms out, swap, ~70ms in |
| Anything ≥ 25 | — | indistinguishable from no animation |

Positional movement should always be faster than the colour change riding on
it, otherwise the widget looks like it is dragging.

## Cross-tab fade

Alpha falls to zero, *then* the page swaps, then alpha climbs back. Swapping
first is the usual mistake and makes both pages visible at once.

```cpp
static float tab_alpha = 1.f;
static int   active    = 0;
ui::Approach(tab_alpha, (tab == active) ? 1.f : 0.f, theme.motion.tab_fade);
if (tab_alpha < 0.02f) active = tab;

ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha * ImGui::GetStyle().Alpha);
DrawPage(active);
ImGui::PopStyleVar();
```

Multiply by `GetStyle().Alpha` rather than assigning, so the fade composes with
any outer fade instead of overriding it.

## Sliding indicators

For a tab bar with one indicator that travels between tabs, store the target
rectangle on the *bar's* ID, not on each tab:

```cpp
ui::AnimState& bar = ui::GetAnim(window->GetID("##tabbar"));
ui::Approach(bar.f[0], selected_tab_x,     16.f);
ui::Approach(bar.f[1], selected_tab_width, 16.f);
draw->AddRectFilled({bar.f[0], y}, {bar.f[0] + bar.f[1], y + 2.f}, accent, 1.f);
```

## What cannot be inferred from a still image

Timing, easing and hover behaviour are invisible in a screenshot. Use the table
above as the default, state that it was assumed, and ask for a short screen
recording if the user wants the motion matched rather than plausible.
