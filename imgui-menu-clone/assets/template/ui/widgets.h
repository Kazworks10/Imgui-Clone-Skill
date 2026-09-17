// ui/widgets.h — the custom widget layer.
//
// HARD RULE: never patch imgui_widgets.cpp. Every custom widget lives here, in
// namespace ui, so the ImGui submodule stays a clean pinned checkout that can
// be upgraded without re-applying a diff.
#pragma once
#include "imgui.h"
#include "theme.h"
#include "anim.h"

namespace ui {

// Call after ImGui::NewFrame().
void NewFrame();

// ─── containers ─────────────────────────────────────────────────────────────
bool BeginGroupBox(const char* title, const ImVec2& size);
void EndGroupBox();

// ─── navigation ─────────────────────────────────────────────────────────────
// `icon` may be NULL, a UTF-8 icon-font glyph (ICON_FA_*), or plain text.
bool Tab(const char* icon, const char* label, bool selected, const ImVec2& size);

// ─── controls ───────────────────────────────────────────────────────────────
bool Toggle(const char* label, bool* v);
bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* fmt = "%d");
bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* fmt = "%.1f");
bool Button(const char* label, const ImVec2& size = ImVec2(0, 0));
bool InputText(const char* id, const char* hint, char* buf, int buf_size, const ImVec2& size = ImVec2(0, 0));
bool Combo(const char* label, int* current_item, const char* const items[], int count);
bool MultiCombo(const char* label, bool* values, const char* const items[], int count);
bool Keybind(const char* label, int* key);          // key is an ImGuiKey
bool ColorPicker(const char* label, float col[4]);
void Separator();
void Text(const char* fmt, ...) IM_FMTARGS(1);

// ─── draw helpers ───────────────────────────────────────────────────────────
// Soft drop shadow. Uses ImDrawList::AddShadowRect when the ImGui build exposes
// it (shadow branch), otherwise an alpha-ramp fallback that needs no core patch.
void ShadowRect(ImDrawList* dl, const ImVec2& mn, const ImVec2& mx,
                ImU32 col, float thickness, float rounding);

const char* KeyName(int key);

} // namespace ui
