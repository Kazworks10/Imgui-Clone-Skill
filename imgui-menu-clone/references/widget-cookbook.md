# Writing a widget that does not exist yet

The template covers Toggle, Slider, Button, InputText, Combo, MultiCombo,
Keybind, ColorPicker, Tab, GroupBox, Separator. When the spec calls for
something else, write it here — never by reshaping a shared widget.

## The skeleton every widget follows

```cpp
bool ui::Thing(const char* label, T* value)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems) return false;                       // 1. clipped out

    const ImGuiID id  = win->GetID(label);                  // 2. identity
    const ImVec2  pos = win->DC.CursorPos;
    const ImVec2  size(RowWidth(), computed_height);
    const ImRect  bb(pos, pos + size);

    ImGui::ItemSize(size);                                  // 3. reserve layout
    if (!ImGui::ItemAdd(bb, id)) return false;              // 4. register + clip

    bool hovered, held;                                     // 5. interaction
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) { /* mutate *value */ ImGui::MarkItemEdited(id); }

    ui::AnimState& st = ui::GetAnim(id);                    // 6. animation
    ui::Approach(st.c[0], hovered ? a : b, ui::theme.motion.speed);

    win->DrawList->Add...                                   // 7. draw
    return pressed;
}
```

Steps 3–5 are non-negotiable. Skipping `ItemAdd` and testing
`IsMouseHoveringRect` yourself breaks the moment a popup overlaps the widget,
the window is clipped, or two widgets share a row.

`ItemSize` before `ItemAdd`, always. Reversed, the layout cursor is wrong by one
widget and the whole panel drifts.

Needs `#include "imgui_internal.h"` (with `IMGUI_DEFINE_MATH_OPERATORS` defined
first, for `pos + size`).

## Shape recipes

Only the drawing differs between designs. All coordinates below assume `bb`.

**Circular toggle knob** (instead of the template's pill)
```cpp
const float r = h * 0.5f - pad;
const float cx = ImLerp(mn.x + pad + r, mx.x - pad - r, t);
dl->AddRectFilled(mn, mx, track, h * 0.5f);
dl->AddCircleFilled(ImVec2(cx, (mn.y + mx.y) * 0.5f), r, knob, 24);
```

**Checkbox with a tick** — the tick is two lines, not a glyph:
```cpp
dl->AddRectFilled(mn, mx, fill, rounding);
const ImVec2 c = (mn + mx) * 0.5f; const float s = (mx.x - mn.x) * 0.22f;
dl->PathLineTo({c.x - s, c.y});
dl->PathLineTo({c.x - s * 0.2f, c.y + s * 0.8f});
dl->PathLineTo({c.x + s, c.y - s * 0.8f});
dl->PathStroke(tick_col, 0, 2.f);
```
Animate the tick by scaling `s` from 0, or by clipping with `PushClipRect`.

**Radio**: `AddCircle` outline + `AddCircleFilled` inner dot, dot radius lerped.

**Dropdown caret**: a triangle `AddTriangleFilled`, or the three-bar glyph used
in the template (`AddLine` x3). Rotate a caret by lerping the three points.

**Top tab bar with sliding indicator**: see `animation.md` — the indicator's
state belongs to the bar, not the tabs.

**Gradient header / accent wash**:
```cpp
dl->AddRectFilledMultiColor(mn, mx, c_tl, c_tr, c_br, c_bl);
```
Note it ignores rounding. For a rounded gradient, clip first:
`dl->PushClipRect(mn, mx, true)` inside a rounded `AddRectFilled`, or draw the
gradient then overdraw the corners with the background colour.

**Glow around an element** — concentric rounded rects with a quadratic alpha
ramp; this is what `ui::ShadowRect` does. Use the accent colour rather than
black and offset by zero.

**Progress arc / circular meter**:
```cpp
dl->PathArcTo(centre, radius, -IM_PI * 0.5f, -IM_PI * 0.5f + IM_PI * 2.f * t, 64);
dl->PathStroke(accent, 0, thickness);
```

**Sparkline / graph**: `AddPolyline` over a `ImVec2` array; grid lines with
`AddLine` at fixed fractions; fill under the curve with
`AddConvexPolyFilled` on the closed polygon.

**Scrollbar restyle**: do not draw your own. Push
`ImGuiCol_ScrollbarBg/Grab/GrabHovered/GrabActive` and
`ImGuiStyleVar_ScrollbarRounding`, and set `style.ScrollbarSize`.

**Badge / pill**: `AddRectFilled` with `rounding = h * 0.5f`, text centred with
`CalcTextSize`. Width = text width + 2 * padding, never a fixed number.

**Text that must be clipped**: `ImGui::RenderTextClipped(min, max, text, NULL,
&size, align)` — handles ellipsis-free clipping to a rect. Plain
`AddText` overflows its box.

## Layout facts worth knowing

- Full row width inside a panel: `ImGui::GetContentRegionAvail().x`. Do not use
  `GetWindowWidth()` minus padding — it is wrong inside a child with a
  scrollbar.
- Right-align a control: `bb.Max.x - control_w`. Right-align text: subtract
  `ImGui::CalcTextSize(txt).x`.
- Vertically centre anything of height `hh` in a row of height `h`:
  `pos.y + (h - hh) * 0.5f`.
- `ImGui::SameLine(0.f, spacing)` — passing `0.f` as the first argument means
  "no absolute position", not "no spacing".
- A panel's inner content height must subtract its header and both paddings, or
  the last row is clipped and looks like a missing widget.

## Fidelity check before moving on

For every new widget ask: does it react to hover? does it animate in both
directions? does it still look right at a different accent colour? Menus are
almost always recoloured live from a picker, and a widget with a hardcoded
accent is the one that will be spotted.
