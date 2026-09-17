// ui/anim.h — per-widget animation state, keyed by ImGuiID.
//
// Immediate-mode widgets own no memory, so animated values must be parked in a
// side table keyed by the widget's ImGuiID. Never use a function-local
// `static float`: it is shared by every instance of the widget and two toggles
// in the same window will animate as one.
#pragma once
#include "imgui.h"

namespace ui {

struct AnimState
{
    float  f[6] = {};       // scalars  (knob offset, fill %, hover ramp, ...)
    ImVec4 c[4] = {};       // colours  (background, text, accent, ...)
    bool   initialized = false;
    int    last_frame  = -1;
};

// Fetches (or creates) the state for this widget. Entries unused for a few
// seconds are collected by NewFrame(), so hidden tabs do not leak.
AnimState& GetAnim(ImGuiID id);

// Frame-rate independent approach. Clamps the blend factor: on a frame hitch
// DeltaTime * speed can exceed 1.0, and an unclamped ImLerp then overshoots
// and visibly snaps back.
float  Approach(float& v, float target, float speed);
ImVec4 Approach(ImVec4& v, const ImVec4& target, float speed);

// Called once per frame by ui::NewFrame(). Do not call directly.
void AnimNewFrame();

} // namespace ui
