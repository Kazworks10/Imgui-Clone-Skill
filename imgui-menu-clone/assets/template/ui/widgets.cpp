#include "widgets.h"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include <cstdio>
#include <cstdarg>
#include <cstring>

namespace ui {

// ─────────────────────────────────────────────────────────────────────────────
// helpers
// ─────────────────────────────────────────────────────────────────────────────
static inline ImU32 C(const ImVec4& c) { return ImGui::GetColorU32(c); }

static inline float RowWidth()
{
    const float w = ImGui::GetContentRegionAvail().x;
    return (w > 1.f) ? w : 1.f;
}

void NewFrame()
{
    AnimNewFrame();
}

const char* KeyName(int key)
{
    if (key <= ImGuiKey_None)
        return "None";
    return ImGui::GetKeyName(static_cast<ImGuiKey>(key));
}

void ShadowRect(ImDrawList* dl, const ImVec2& mn, const ImVec2& mx,
                ImU32 col, float thickness, float rounding)
{
#ifdef UI_HAS_IMGUI_SHADOWS
    dl->AddShadowRect(mn, mx, col, thickness, ImVec2(0, 0), ImDrawFlags_None, rounding);
#else
    // Fallback: concentric rounded rects with a quadratic alpha ramp. Visually
    // close enough for menu-sized boxes and needs no patch to ImGui core.
    const int   steps = ImMax(2, theme.shadow.steps);
    const float base  = static_cast<float>((col >> IM_COL32_A_SHIFT) & 0xFF);
    for (int i = steps; i >= 1; --i)
    {
        const float t     = static_cast<float>(i) / static_cast<float>(steps);
        const float grow  = thickness * t;
        const float alpha = base * (1.f - t) * (1.f - t) / static_cast<float>(steps) * 2.2f;
        const ImU32 c = (col & ~IM_COL32_A_MASK)
                      | (static_cast<ImU32>(ImClamp(alpha, 0.f, 255.f)) << IM_COL32_A_SHIFT);
        dl->AddRectFilled(mn - ImVec2(grow, grow), mx + ImVec2(grow, grow),
                          c, rounding + grow);
    }
#endif
}

void Text(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, C(theme.text.idle));
    ImGui::TextV(fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

void Separator()
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return;

    const float  w   = RowWidth();
    const ImVec2 pos = win->DC.CursorPos;
    const float  h   = theme.separator.height;

    ImGui::ItemSize(ImVec2(w, h));
    const ImRect bb(pos, pos + ImVec2(w, h));
    if (!ImGui::ItemAdd(bb, 0))
        return;

    const float y = IM_ROUND(pos.y + h * 0.5f);
    win->DrawList->AddLine(ImVec2(pos.x, y), ImVec2(pos.x + w, y),
                           C(theme.separator.line), 1.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// containers
// ─────────────────────────────────────────────────────────────────────────────
static ImVector<ImRect> g_groupbox_stack;

bool BeginGroupBox(const char* title, const ImVec2& size)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImVec2 p  = win->DC.CursorPos;
    const ImVec2 mx = p + size;

    ImDrawList* dl = win->DrawList;
    const float hh = theme.child.header_h;

    ShadowRect(dl, p, mx, C(theme.shadow.color), theme.shadow.thickness, theme.child.rounding);

    // Body is near-transparent so the window (and the blur behind it) reads
    // through; only the header strip is solid. Filling the body opaque flattens
    // the design and is the most common fidelity miss.
    dl->AddRectFilled(p, mx, C(theme.child.bg_body), theme.child.rounding);
    dl->AddRectFilled(p, ImVec2(mx.x, p.y + hh), C(theme.child.bg_header),
                      theme.child.rounding, ImDrawFlags_RoundCornersTop);
    dl->AddRect(p, mx, C(theme.child.border), theme.child.rounding,
                ImDrawFlags_None, theme.child.border_size);

    // The rule under the title is accent-tinted in many designs, not grey.
    ImVec4 rule = theme.child.border;
    if (theme.child.header_line_accent)
        rule = ImVec4(theme.accent.x, theme.accent.y, theme.accent.z,
                      theme.child.header_line_alpha);
    dl->AddLine(ImVec2(p.x, p.y + hh), ImVec2(mx.x, p.y + hh),
                C(rule), theme.child.border_size);

    if (title && *title)
    {
        const ImVec2 ts = ImGui::CalcTextSize(title, nullptr, true);
        dl->AddText(ImVec2(p.x + (size.x - ts.x) * 0.5f, p.y + (hh - ts.y) * 0.5f),
                    C(theme.child.header_text), title);
    }

    g_groupbox_stack.push_back(ImRect(p, mx));

    const float pad = theme.child.pad;
    ImGui::SetCursorScreenPos(ImVec2(p.x + pad, p.y + hh + pad));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 10.f));
    return ImGui::BeginChild(title,
                             ImVec2(size.x - pad * 2.f, size.y - hh - pad * 2.f),
                             ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoBackground);
}

void EndGroupBox()
{
    ImGui::EndChild();
    ImGui::PopStyleVar();

    IM_ASSERT(!g_groupbox_stack.empty() && "EndGroupBox without BeginGroupBox");
    const ImRect box = g_groupbox_stack.back();
    g_groupbox_stack.pop_back();

    ImGui::SetCursorScreenPos(box.Min);
    ImGui::Dummy(box.GetSize());
}

// ─────────────────────────────────────────────────────────────────────────────
// navigation
// ─────────────────────────────────────────────────────────────────────────────
bool Tab(const char* icon, const char* label, bool selected, const ImVec2& size)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    const ImGuiID id  = win->GetID(label);
    const ImVec2  pos = win->DC.CursorPos;
    const ImRect  bb(pos, pos + size);

    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    AnimState& st = GetAnim(id);
    if (!st.initialized)
    {
        st.initialized = true;
        st.c[0] = theme.accent_off;
        st.c[1] = theme.accent_off;
        st.c[2] = theme.tab.text_inactive;
    }
    Approach(st.c[0], selected ? theme.accent_dim : theme.accent_off, theme.motion.speed);
    Approach(st.c[1], selected ? theme.accent     : theme.accent_off, theme.motion.speed);
    Approach(st.c[2], selected ? theme.tab.text_active
                     : (hovered ? theme.text.hover : theme.tab.text_inactive),
             theme.motion.speed_slow);

    ImDrawList* dl = win->DrawList;
    // Accent wash fading out to the right, then the accent bar on the left edge.
    dl->AddRectFilledMultiColor(bb.Min, bb.Max,
                                C(st.c[0]), C(theme.accent_off),
                                C(theme.accent_off), C(st.c[0]));
    dl->AddRectFilled(bb.Min, ImVec2(bb.Min.x + theme.tab.bar_w, bb.Max.y), C(st.c[1]));

    ImGui::PushStyleColor(ImGuiCol_Text, C(st.c[2]));
    if (icon && *icon)
    {
        if (fonts::icon_sm) ImGui::PushFont(fonts::icon_sm);
        const ImVec2 is = ImGui::CalcTextSize(icon);
        dl->AddText(ImVec2(bb.Min.x + theme.tab.icon_x, bb.Min.y + (size.y - is.y) * 0.5f),
                    C(st.c[2]), icon);
        if (fonts::icon_sm) ImGui::PopFont();
    }
    const ImVec2 ls = ImGui::CalcTextSize(label, nullptr, true);
    dl->AddText(ImVec2(bb.Min.x + theme.tab.label_x, bb.Min.y + (size.y - ls.y) * 0.5f),
                C(st.c[2]), label);
    ImGui::PopStyleColor();

    return pressed;
}

// ─────────────────────────────────────────────────────────────────────────────
// controls
// ─────────────────────────────────────────────────────────────────────────────
bool Toggle(const char* label, bool* v)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    const ImGuiID id = win->GetID(label);
    const ImVec2  ls = ImGui::CalcTextSize(label, nullptr, true);
    const float   w  = RowWidth();
    const float   h  = ImMax(ls.y, theme.toggle.knob_h) + 4.f;
    const ImVec2  pos = win->DC.CursorPos;
    const ImRect  bb(pos, pos + ImVec2(w, h));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed)
    {
        *v = !*v;
        ImGui::MarkItemEdited(id);
    }

    AnimState& st = GetAnim(id);
    if (!st.initialized)
    {
        st.initialized = true;
        st.f[0] = *v ? 1.f : 0.f;
        st.c[0] = *v ? theme.accent : theme.toggle.knob_ring_off;
        st.c[1] = *v ? theme.text.active : theme.text.idle;
    }
    Approach(st.f[0], *v ? 1.f : 0.f, theme.motion.speed_fast);
    Approach(st.c[0], *v ? theme.accent : theme.toggle.knob_ring_off, theme.motion.speed);
    Approach(st.c[1], *v ? theme.text.active
                    : (hovered ? theme.text.hover : theme.text.idle), theme.motion.speed_slow);

    ImDrawList* dl = win->DrawList;

    // Track: stays dark in both states — it never fills with the accent.
    const float  tw = theme.toggle.track_w, tth = theme.toggle.track_h;
    const ImVec2 tmn(bb.Max.x - tw, bb.Min.y + (h - tth) * 0.5f);
    const ImVec2 tmx(tmn.x + tw, tmn.y + tth);
    dl->AddRectFilled(tmn, tmx, C(theme.toggle.track), tth * 0.5f);
    dl->AddRect(tmn, tmx, C(theme.toggle.track_border), tth * 0.5f, ImDrawFlags_None, 1.f);

    // Knob: a ring (1px coloured outline over a dark fill) that is taller than
    // the track and overhangs it. Drawn as fill + inset fill, not AddRect,
    // so the outline stays crisp at any rounding.
    const float  kw = theme.toggle.knob_w, kh = theme.toggle.knob_h;
    const float  kx = tmn.x + st.f[0] * theme.toggle.travel;
    const ImVec2 kmn(kx, bb.Min.y + (h - kh) * 0.5f);
    const ImVec2 kmx(kx + kw, kmn.y + kh);
    const float  ring = theme.toggle.ring;
    dl->AddRectFilled(kmn, kmx, C(st.c[0]), kh * 0.5f);
    dl->AddRectFilled(kmn + ImVec2(ring, ring), kmx - ImVec2(ring, ring),
                      C(theme.toggle.knob_fill), kh * 0.5f);

    dl->AddText(ImVec2(bb.Min.x, bb.Min.y + (h - ls.y) * 0.5f), C(st.c[1]), label);
    return pressed;
}

// Shared slider body. `t` in/out is the normalised position.
static bool SliderBody(const char* label, const char* value_text, float* t)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    const ImGuiID id = win->GetID(label);
    const ImVec2  ls = ImGui::CalcTextSize(label, nullptr, true);
    const float   w  = RowWidth();
    // The knob is taller than the track and overhangs the item rect. That is
    // intentional: reserving room for it would break the measured row pitch.
    const float   h  = ls.y + theme.slider.label_gap + theme.slider.track_h;
    const ImVec2  pos = win->DC.CursorPos;
    const ImRect  bb(pos, pos + ImVec2(w, h));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    const ImVec2 tmn(bb.Min.x, bb.Max.y - theme.slider.track_h);
    const ImVec2 tmx(bb.Max.x, bb.Max.y);

    bool hovered, held;
    ImGui::ButtonBehavior(ImRect(ImVec2(tmn.x, tmn.y - 8.f), ImVec2(tmx.x, tmx.y + 8.f)),
                          id, &hovered, &held);

    bool changed = false;
    if (held)
    {
        const float nt = ImSaturate((ImGui::GetIO().MousePos.x - tmn.x) / ImMax(1.f, tmx.x - tmn.x));
        if (nt != *t) { *t = nt; changed = true; ImGui::MarkItemEdited(id); }
    }

    AnimState& st = GetAnim(id);
    if (!st.initialized) { st.initialized = true; st.f[0] = *t; st.c[0] = theme.text.idle; }
    Approach(st.f[0], *t, theme.motion.speed * 2.f);
    Approach(st.c[0], (hovered || held) ? theme.text.active : theme.text.idle,
             theme.motion.speed_slow);

    ImDrawList* dl = win->DrawList;
    dl->AddText(ImVec2(bb.Min.x, bb.Min.y), C(st.c[0]), label);

    const ImVec2 vs = ImGui::CalcTextSize(value_text);
    dl->AddText(ImVec2(bb.Max.x - vs.x, bb.Min.y), C(theme.text.active), value_text);

    const float kw = theme.slider.knob_w;
    const float kh = theme.slider.knob_h;
    const float kx = ImLerp(tmn.x, tmx.x - kw, st.f[0]);
    const float cy = (tmn.y + tmx.y) * 0.5f;

    dl->AddRectFilled(tmn, tmx, C(theme.slider.track), theme.slider.rounding);

    // Accent fill runs from the track start to the knob's leading edge, so it
    // is invisible at zero and never peeks out from under the knob.
    if (theme.slider.fill_track && kx > tmn.x + 1.f)
        dl->AddRectFilled(tmn, ImVec2(kx, tmx.y), C(theme.accent), theme.slider.rounding);

    dl->AddRect(tmn, tmx, C(theme.slider.track_border), theme.slider.rounding,
                ImDrawFlags_None, 1.f);

    // Ring knob, same construction as the toggle.
    const ImVec2 kmn(kx, cy - kh * 0.5f);
    const ImVec2 kmx(kx + kw, cy + kh * 0.5f);
    const float  ring = theme.slider.ring;
    dl->AddRectFilled(kmn, kmx, C(theme.accent), kh * 0.5f);
    dl->AddRectFilled(kmn + ImVec2(ring, ring), kmx - ImVec2(ring, ring),
                      C(theme.slider.knob_fill), kh * 0.5f);

    return changed;
}

bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* fmt)
{
    char buf[64];
    ImFormatString(buf, IM_ARRAYSIZE(buf), fmt, *v);

    const float span = static_cast<float>(v_max - v_min);
    float t = (span > 0.f) ? (static_cast<float>(*v - v_min) / span) : 0.f;

    if (!SliderBody(label, buf, &t))
        return false;

    const int nv = v_min + static_cast<int>(IM_ROUND(t * span));
    if (nv == *v)
        return false;
    *v = nv;
    return true;
}

bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* fmt)
{
    char buf[64];
    ImFormatString(buf, IM_ARRAYSIZE(buf), fmt, *v);

    const float span = v_max - v_min;
    float t = (span != 0.f) ? ((*v - v_min) / span) : 0.f;

    if (!SliderBody(label, buf, &t))
        return false;

    *v = v_min + t * span;
    return true;
}

bool Button(const char* label, const ImVec2& size)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    const ImGuiID id = win->GetID(label);
    const ImVec2  sz(size.x > 0.f ? size.x : RowWidth(),
                     size.y > 0.f ? size.y : theme.frame.height);
    const ImVec2  pos = win->DC.CursorPos;
    const ImRect  bb(pos, pos + sz);

    ImGui::ItemSize(sz);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    AnimState& st = GetAnim(id);
    if (!st.initialized) { st.initialized = true; st.c[0] = theme.frame.bg; }
    Approach(st.c[0], hovered ? theme.frame.bg_hover : theme.frame.bg, theme.motion.speed);

    ImDrawList* dl = win->DrawList;
    dl->AddRectFilled(bb.Min, bb.Max, C(st.c[0]), theme.frame.rounding);
    dl->AddRect(bb.Min, bb.Max, C(hovered ? theme.accent : theme.frame.border),
                theme.frame.rounding, ImDrawFlags_None, theme.frame.border_sz);

    const ImVec2 ts = ImGui::CalcTextSize(label, nullptr, true);
    dl->AddText(ImVec2(bb.Min.x + (sz.x - ts.x) * 0.5f, bb.Min.y + (sz.y - ts.y) * 0.5f),
                C(theme.text.active), label);
    return pressed;
}

bool InputText(const char* id, const char* hint, char* buf, int buf_size, const ImVec2& size)
{
    const ImVec2 sz(size.x > 0.f ? size.x : RowWidth(),
                    size.y > 0.f ? size.y : theme.frame.height);

    ImGui::PushStyleColor(ImGuiCol_FrameBg,        C(theme.frame.bg));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, C(theme.frame.bg_hover));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  C(theme.frame.bg_hover));
    ImGui::PushStyleColor(ImGuiCol_Border,         C(theme.frame.border));
    ImGui::PushStyleColor(ImGuiCol_Text,           C(theme.text.active));
    ImGui::PushStyleColor(ImGuiCol_TextDisabled,   C(theme.text.idle));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,  theme.frame.rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, theme.frame.border_sz);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(10.f, ImMax(0.f, (sz.y - ImGui::GetFontSize()) * 0.5f)));

    const bool changed = ImGui::InputTextEx(id, hint, buf, buf_size, sz, ImGuiInputTextFlags_None);

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(6);
    return changed;
}

// Frame + caret used by Combo / MultiCombo.
static bool ComboFrame(const char* label, const char* preview, ImGuiID id, ImRect* out_bb)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();

    const ImVec2 ls = ImGui::CalcTextSize(label, nullptr, true);
    const float  w  = RowWidth();
    const float  h  = ls.y + theme.frame.label_gap + theme.frame.height;
    const ImVec2 pos = win->DC.CursorPos;
    const ImRect bb(pos, pos + ImVec2(w, h));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    const ImRect frame(ImVec2(bb.Min.x, bb.Max.y - theme.frame.height), bb.Max);

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(frame, id, &hovered, &held);

    AnimState& st = GetAnim(id);
    if (!st.initialized) { st.initialized = true; st.c[0] = theme.frame.bg; }
    Approach(st.c[0], hovered ? theme.frame.bg_hover : theme.frame.bg, theme.motion.speed);

    ImDrawList* dl = win->DrawList;
    dl->AddText(bb.Min, C(theme.text.idle), label);
    dl->AddRectFilled(frame.Min, frame.Max, C(st.c[0]), theme.frame.rounding);
    dl->AddRect(frame.Min, frame.Max, C(hovered ? theme.accent : theme.frame.border),
                theme.frame.rounding, ImDrawFlags_None, theme.frame.border_sz);

    const ImVec2 ps = ImGui::CalcTextSize(preview);
    dl->AddText(ImVec2(frame.Min.x + theme.frame.text_pad,
                       frame.Min.y + (theme.frame.height - ps.y) * 0.5f),
                C(theme.text.idle), preview);

    // Three-bar caret on the right.
    const float cx = frame.Max.x - theme.caret.inset - theme.caret.width;
    const float cy = (frame.Min.y + frame.Max.y) * 0.5f;
    for (int i = -1; i <= 1; ++i)
    {
        const float y = IM_ROUND(cy + i * theme.caret.gap);
        dl->AddLine(ImVec2(cx, y), ImVec2(cx + theme.caret.width, y),
                    C(theme.text.idle), theme.caret.thickness);
    }

    *out_bb = frame;
    return pressed;
}

static void PushPopupStyle()
{
    ImGui::PushStyleColor(ImGuiCol_PopupBg, C(theme.popup.bg));
    ImGui::PushStyleColor(ImGuiCol_Border,  C(theme.popup.border));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   theme.popup.rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(6.f, 6.f));
}
static void PopPopupStyle() { ImGui::PopStyleVar(3); ImGui::PopStyleColor(2); }

// Row inside a combo popup. Returns true when clicked.
static bool PopupRow(const char* label, bool selected)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImGuiID id = win->GetID(label);
    const ImVec2  sz(ImGui::GetContentRegionAvail().x, theme.popup.item_h);
    const ImVec2  pos = win->DC.CursorPos;
    const ImRect  bb(pos, pos + sz);

    ImGui::ItemSize(sz);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    AnimState& st = GetAnim(id);
    if (!st.initialized) { st.initialized = true; st.c[0] = theme.text.idle; }
    Approach(st.c[0], selected ? theme.accent : (hovered ? theme.text.active : theme.text.idle),
             theme.motion.speed);

    if (hovered)
        win->DrawList->AddRectFilled(bb.Min, bb.Max, C(theme.popup.hovered), 4.f);

    const ImVec2 ts = ImGui::CalcTextSize(label, nullptr, true);
    win->DrawList->AddText(ImVec2(bb.Min.x + 8.f, bb.Min.y + (sz.y - ts.y) * 0.5f),
                           C(st.c[0]), label);
    return pressed;
}

bool Combo(const char* label, int* current_item, const char* const items[], int count)
{
    ImGui::PushID(label);
    ImGuiWindow* win  = ImGui::GetCurrentWindow();
    const ImGuiID id  = win->GetID(label);
    const char* preview = (*current_item >= 0 && *current_item < count) ? items[*current_item] : "";

    ImRect frame;
    if (ComboFrame(label, preview, id, &frame))
        ImGui::OpenPopup("##combo");

    bool changed = false;
    ImGui::SetNextWindowPos(ImVec2(frame.Min.x, frame.Max.y + 4.f));
    ImGui::SetNextWindowSize(ImVec2(frame.GetWidth(), 0.f));
    PushPopupStyle();
    if (ImGui::BeginPopup("##combo", ImGuiWindowFlags_NoMove))
    {
        for (int i = 0; i < count; ++i)
            if (PopupRow(items[i], i == *current_item))
            {
                *current_item = i;
                changed = true;
                ImGui::CloseCurrentPopup();
            }
        ImGui::EndPopup();
    }
    PopPopupStyle();
    ImGui::PopID();
    return changed;
}

bool MultiCombo(const char* label, bool* values, const char* const items[], int count)
{
    ImGui::PushID(label);
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImGuiID id = win->GetID(label);

    const char* ell = theme.combo.ellipsis;
    const int   cap = theme.combo.max_preview_items;

    char preview[256] = {};
    int  written = 0, shown = 0, selected = 0;
    for (int i = 0; i < count; ++i)
    {
        if (!values[i])
            continue;
        ++selected;
        if (cap > 0 && shown >= cap)
            continue;
        written += ImFormatString(preview + written, IM_ARRAYSIZE(preview) - written,
                                  "%s%s", written ? ", " : "", items[i]);
        ++shown;
    }
    if (!written)
        ImStrncpy(preview, "None", 5);

    if (cap > 0)
    {
        // Count-based: append the marker as soon as anything was left out.
        if (selected > shown)
            written += ImFormatString(preview + written, IM_ARRAYSIZE(preview) - written,
                                      "%s", ell);
    }
    else
    {
        // Width-based: trim until it fits. Robust to label, font-size and
        // panel-width changes in a way a fixed character count is not.
        const float avail = RowWidth() - theme.frame.text_pad
                          - theme.caret.inset - theme.caret.width - 8.f;
        if (ImGui::CalcTextSize(preview).x > avail)
        {
            const float ew = ImGui::CalcTextSize(ell).x;
            while (written > 1 && ImGui::CalcTextSize(preview).x + ew > avail)
                preview[--written] = '\0';
            ImStrncpy(preview + written, ell, IM_ARRAYSIZE(preview) - written);
        }
    }

    ImRect frame;
    if (ComboFrame(label, preview, id, &frame))
        ImGui::OpenPopup("##multi");

    bool changed = false;
    ImGui::SetNextWindowPos(ImVec2(frame.Min.x, frame.Max.y + 4.f));
    ImGui::SetNextWindowSize(ImVec2(frame.GetWidth(), 0.f));
    PushPopupStyle();
    if (ImGui::BeginPopup("##multi", ImGuiWindowFlags_NoMove))
    {
        for (int i = 0; i < count; ++i)
            if (PopupRow(items[i], values[i]))
            {
                values[i] = !values[i];
                changed = true;
            }
        ImGui::EndPopup();
    }
    PopPopupStyle();
    ImGui::PopID();
    return changed;
}

bool Keybind(const char* label, int* key)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems)
        return false;

    const ImGuiID id = win->GetID(label);
    const ImVec2  ls = ImGui::CalcTextSize(label, nullptr, true);
    const float   w  = RowWidth();
    const float   bh = 24.f;
    const float   h  = ImMax(ls.y, bh) + 4.f;
    const ImVec2  pos = win->DC.CursorPos;
    const ImRect  bb(pos, pos + ImVec2(w, h));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    AnimState& st = GetAnim(id);
    const bool listening = (st.f[1] > 0.5f);

    const char* txt = listening ? "..." : KeyName(*key);
    const float bw  = ImMax(52.f, ImGui::CalcTextSize(txt).x + 20.f);
    const ImRect box(ImVec2(bb.Max.x - bw, bb.Min.y + (h - bh) * 0.5f),
                     ImVec2(bb.Max.x,      bb.Min.y + (h - bh) * 0.5f + bh));

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(box, id, &hovered, &held);
    if (pressed)
        st.f[1] = 1.f;

    bool changed = false;
    if (listening)
    {
        for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k)
        {
            if (!ImGui::IsKeyPressed(static_cast<ImGuiKey>(k), false))
                continue;
            *key = (k == ImGuiKey_Escape || k == ImGuiKey_Delete) ? ImGuiKey_None : k;
            st.f[1] = 0.f;
            changed = true;
            ImGui::MarkItemEdited(id);
            break;
        }
    }

    if (!st.initialized) { st.initialized = true; st.c[0] = theme.frame.bg; }
    Approach(st.c[0], (hovered || listening) ? theme.frame.bg_hover : theme.frame.bg,
             theme.motion.speed);

    ImDrawList* dl = win->DrawList;
    dl->AddText(ImVec2(bb.Min.x, bb.Min.y + (h - ls.y) * 0.5f), C(theme.text.idle), label);
    dl->AddRectFilled(box.Min, box.Max, C(st.c[0]), theme.frame.rounding);
    dl->AddRect(box.Min, box.Max, C(listening ? theme.accent : theme.frame.border),
                theme.frame.rounding, ImDrawFlags_None, theme.frame.border_sz);

    const ImVec2 ts = ImGui::CalcTextSize(txt);
    dl->AddText(ImVec2(box.Min.x + (bw - ts.x) * 0.5f, box.Min.y + (bh - ts.y) * 0.5f),
                C(theme.text.active), txt);
    return changed;
}

bool ColorPicker(const char* label, float col[4])
{
    ImGui::PushID(label);
    ImGuiWindow* win = ImGui::GetCurrentWindow();

    const ImGuiID id = win->GetID(label);
    const ImVec2  ls = ImGui::CalcTextSize(label, nullptr, true);
    const float   w  = RowWidth();
    const float   sw = theme.swatch.w, sh = theme.swatch.h;
    const float   h  = ImMax(ls.y, sh) + 4.f;
    const ImVec2  pos = win->DC.CursorPos;
    const ImRect  bb(pos, pos + ImVec2(w, h));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
    {
        ImGui::PopID();
        return false;
    }

    const ImRect sw_bb(ImVec2(bb.Max.x - sw, bb.Min.y + (h - sh) * 0.5f),
                       ImVec2(bb.Max.x,      bb.Min.y + (h - sh) * 0.5f + sh));

    bool hovered, held;
    if (ImGui::ButtonBehavior(sw_bb, id, &hovered, &held))
        ImGui::OpenPopup("##picker");

    ImDrawList* dl = win->DrawList;
    dl->AddText(ImVec2(bb.Min.x, bb.Min.y + (h - ls.y) * 0.5f), C(theme.text.idle), label);
    dl->AddRectFilled(sw_bb.Min, sw_bb.Max,
                      ImGui::GetColorU32(ImVec4(col[0], col[1], col[2], col[3])),
                      theme.swatch.rounding);
    dl->AddRect(sw_bb.Min, sw_bb.Max, C(hovered ? theme.accent : theme.frame.border),
                theme.swatch.rounding, ImDrawFlags_None, theme.frame.border_sz);

    bool changed = false;
    PushPopupStyle();
    if (ImGui::BeginPopup("##picker"))
    {
        changed = ImGui::ColorPicker4("##p", col,
                                      ImGuiColorEditFlags_NoSidePreview |
                                      ImGuiColorEditFlags_NoInputs |
                                      ImGuiColorEditFlags_AlphaBar);
        ImGui::EndPopup();
    }
    PopPopupStyle();
    ImGui::PopID();
    return changed;
}

} // namespace ui
