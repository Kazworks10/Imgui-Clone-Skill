#include "anim.h"
#include "theme.h"

// ImLerp / ImSaturate / ImFabs live in imgui_internal.h, not imgui.h.
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include <unordered_map>

namespace ui {

static std::unordered_map<ImGuiID, AnimState> g_anim;
static int g_frame = 0;

AnimState& GetAnim(ImGuiID id)
{
    AnimState& st = g_anim[id];
    st.last_frame = g_frame;
    return st;
}

float Approach(float& v, float target, float speed)
{
    const float a = ImSaturate(ImGui::GetIO().DeltaTime * speed);
    v = ImLerp(v, target, a);
    if (ImFabs(target - v) < 0.001f) v = target;   // settle exactly
    return v;
}

ImVec4 Approach(ImVec4& v, const ImVec4& target, float speed)
{
    const float a = ImSaturate(ImGui::GetIO().DeltaTime * speed);
    v = ImLerp(v, target, a);
    return v;
}

void AnimNewFrame()
{
    g_frame = ImGui::GetFrameCount();

    // Cheap amortised GC: sweep every 600 frames, drop anything unseen for 300.
    if ((g_frame % 600) != 0)
        return;
    for (auto it = g_anim.begin(); it != g_anim.end(); )
        it = (g_frame - it->second.last_frame > 300) ? g_anim.erase(it) : ++it;
}

} // namespace ui
