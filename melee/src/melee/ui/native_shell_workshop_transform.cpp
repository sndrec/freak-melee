#include "native_shell_workshop_transform.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>

namespace {
    constexpr unsigned MaxBones = 512;
    constexpr float Pi = 3.14159265358979323846f;
    struct V {
        float x, y, z;
        float& operator[](unsigned i)
        {
            return (&x)[i];
        }
        float operator[](unsigned i) const
        {
            return (&x)[i];
        }
    };
    V operator+(V a, V b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }
    V operator-(V a, V b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }
    V operator*(V a, float b)
    {
        return { a.x * b, a.y * b, a.z * b };
    }
    float dot(V a, V b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    V cross(V a, V b)
    {
        return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                 a.x * b.y - a.y * b.x };
    }
    float length(V a)
    {
        return std::sqrt(dot(a, a));
    }
    V unit(V a)
    {
        float n = length(a);
        return n > 1e-8f ? a * (1.0f / n) : V{ 0, 0, 1 };
    }
    struct M {
        float v[3][4];
    };
    M identity()
    {
        return { { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 } } };
    }
    V point(const M& m, V p)
    {
        return { dot({ m.v[0][0], m.v[0][1], m.v[0][2] }, p) + m.v[0][3],
                 dot({ m.v[1][0], m.v[1][1], m.v[1][2] }, p) + m.v[1][3],
                 dot({ m.v[2][0], m.v[2][1], m.v[2][2] }, p) + m.v[2][3] };
    }
    V origin(const M& m)
    {
        return { m.v[0][3], m.v[1][3], m.v[2][3] };
    }
    M multiply(const M& a, const M& b)
    {
        M r = {};
        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned j = 0; j < 4; ++j) {
                for (unsigned k = 0; k < 3; ++k) {
                    r.v[i][j] += a.v[i][k] * b.v[k][j];
                }
                if (j == 3) {
                    r.v[i][j] += a.v[i][3];
                }
            }
        }
        return r;
    }
    bool inverse(const M& a, M& r)
    {
        V x = { a.v[0][0], a.v[1][0], a.v[2][0] },
          y = { a.v[0][1], a.v[1][1], a.v[2][1] },
          z = { a.v[0][2], a.v[1][2], a.v[2][2] };
        float d = dot(x, cross(y, z));
        if (std::fabs(d) < 1e-9f) {
            return false;
        }
        V rows[] = { cross(y, z) * (1 / d), cross(z, x) * (1 / d),
                     cross(x, y) * (1 / d) };
        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned j = 0; j < 3; ++j) {
                r.v[i][j] = rows[i][j];
            }
            r.v[i][3] = -dot(rows[i], origin(a));
        }
        return true;
    }
    M local_matrix(const NativeHomeWorkshopBoneLocal& p)
    {
        float x = p.rotation[0], y = p.rotation[1], z = p.rotation[2];
        float cx = std::cos(x), sx = std::sin(x), cy = std::cos(y),
              sy = std::sin(y), cz = std::cos(z), sz = std::sin(z);
        M r = {
            { { cz * cy, cz * sy * sx - sz * cx, cz * sy * cx + sz * sx, 0 },
              { sz * cy, sz * sy * sx + cz * cx, sz * sy * cx - cz * sx, 0 },
              { -sy, cy * sx, cy * cx, 0 } }
        };
        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned j = 0; j < 3; ++j) {
                r.v[i][j] *= p.scale[j];
            }
            r.v[i][3] = p.position[i];
        }
        return r;
    }
    NativeHomeWorkshopBoneLocal
    decompose(const M& m, const NativeHomeWorkshopBoneLocal& reference)
    {
        NativeHomeWorkshopBoneLocal p = reference;
        M r = m;
        for (unsigned j = 0; j < 3; ++j) {
            float s = length({ m.v[0][j], m.v[1][j], m.v[2][j] });
            if (reference.scale[j] < 0) {
                s = -s;
            }
            p.scale[j] = s;
            p.position[j] = m.v[j][3];
            for (unsigned i = 0; i < 3; ++i) {
                r.v[i][j] /= std::fabs(s) > 1e-8f ? s : 1;
            }
        }
        p.rotation[1] = std::asin(std::clamp(-r.v[2][0], -1.0f, 1.0f));
        if (std::fabs(std::cos(p.rotation[1])) > 1e-5f) {
            p.rotation[0] = std::atan2(r.v[2][1], r.v[2][2]);
            p.rotation[2] = std::atan2(r.v[1][0], r.v[0][0]);
        } else {
            p.rotation[0] = std::atan2(-r.v[1][2], r.v[1][1]);
            p.rotation[2] = 0;
        }
        for (unsigned i = 0; i < 3; ++i) {
            p.rotation[i] +=
                2 * Pi *
                std::round((reference.rotation[i] - p.rotation[i]) / (2 * Pi));
        }
        return p;
    }

    Rml::ElementDocument* document;
    Rml::Element* viewport;
    Rml::Element* overlay;
    struct Segment {
        Rml::Element* element;
        float x, y, width, angle;
        bool visible, selected, initialized;
    } segments[3][32];
    Rml::Element* tool_buttons[3];
    Rml::Element* hint;
    int drawn_operation = -1, drawn_running = -1;
    NativeWorkshopTransformCommit commit;
    unsigned selected[MaxBones], count, active;
    uint64_t revision;
    NativeWorkshopTransformChange changes[MaxBones];
    M start_world[MaxBones], desired_world[MaxBones];
    unsigned apply_order[MaxBones];
    enum Operation {
        Move,
        Rotate,
        Scale
    } operation = Move;
    enum Pivot {
        Center,
        Active,
        Individual
    } pivot_mode = Center;
    bool running, local_axes, plane, gizmo_drag, suppress_click;
    int axis = -1, mouse_x, mouse_y, start_x, start_y;
    V pivot, basis[3], view_normal, start_on_plane;
    float pivot_x, pivot_y;

    void rect(float& left, float& top, float& width, float& height)
    {
        const auto offset = viewport->GetAbsoluteOffset(Rml::BoxArea::Content);
        float w = (float) viewport->GetClientWidth(),
              h = (float) viewport->GetClientHeight();
        height = std::min(h, w * .75f);
        width = height * 4 / 3;
        left = offset.x + (w - width) * .5f;
        top = offset.y + (h - height) * .5f;
    }
    bool ray(int x, int y, V& o, V& d)
    {
        float l, t, w, h;
        rect(l, t, w, h);
        return w > 0 && h > 0 &&
               nativeHome_WorkshopRay((x - l) / w, (y - t) / h, &o.x, &d.x);
    }
    bool screen(V p, float& x, float& y)
    {
        NativeHomeWorkshopBoneScreen s;
        if (!nativeHome_WorkshopWorldScreen(&p.x, &s)) {
            return false;
        }
        float l, t, w, h;
        rect(l, t, w, h);
        x = l + s.x * w;
        y = t + s.y * h;
        return true;
    }
    bool over_viewport()
    {
        if (!viewport) {
            return false;
        }
        auto p = viewport->GetAbsoluteOffset(Rml::BoxArea::Content);
        return mouse_x >= p.x && mouse_y >= p.y &&
               mouse_x < p.x + viewport->GetClientWidth() &&
               mouse_y < p.y + viewport->GetClientHeight();
    }
    V intersect(int x, int y)
    {
        V o, d;
        if (!ray(x, y, o, d)) {
            return pivot;
        }
        float denom = dot(d, view_normal);
        return std::fabs(denom) > 1e-6f
                   ? o + d * (dot(pivot - o, view_normal) / denom)
                   : pivot;
    }
    void axes_and_pivot()
    {
        pivot = {};
        M m = identity(), a = identity();
        for (unsigned i = 0; i < count; ++i) {
            nativeHome_WorkshopBoneMatrix(selected[i], m.v);
            pivot = pivot + origin(m);
            if (selected[i] == active) {
                a = m;
            }
        }
        pivot = pivot * (1.0f / count);
        if (pivot_mode == Active) {
            pivot = origin(a);
        }
        for (unsigned i = 0; i < 3; ++i) {
            basis[i] = {};
            basis[i][i] = 1;
            if (local_axes) {
                basis[i] = unit({ a.v[0][i], a.v[1][i], a.v[2][i] });
            }
        }
    }
    void finish(bool accept)
    {
        if (!running) {
            return;
        }
        if (!accept || (commit && !commit(changes, count))) {
            for (unsigned i = 0; i < count; ++i) {
                nativeHome_WorkshopSetBoneLocal(changes[i].joint,
                                                &changes[i].before);
            }
        }
        running = false;
        gizmo_drag = false;
    }
    void begin(Operation op, int restriction, bool drag)
    {
        if (!viewport || !count || running) {
            return;
        }
        operation = op;
        axis = restriction;
        plane = false;
        gizmo_drag = drag;
        axes_and_pivot();
        screen(pivot, pivot_x, pivot_y);
        for (unsigned i = 0; i < count; ++i) {
            changes[i].joint = selected[i];
            if (!nativeHome_WorkshopBoneLocal(selected[i],
                                              &changes[i].before) ||
                !nativeHome_WorkshopBoneMatrix(selected[i], start_world[i].v))
            {
                return;
            }
            changes[i].after = changes[i].before;
            apply_order[i] = i;
        }
        // Runtime joints are preorder, so parents are applied before
        // descendants.
        std::sort(
            apply_order, apply_order + count,
            [](unsigned a, unsigned b) { return selected[a] < selected[b]; });
        start_x = mouse_x;
        start_y = mouse_y;
        V o, d;
        if (!ray((int) pivot_x, (int) pivot_y, o, d)) {
            return;
        }
        view_normal = unit(d);
        start_on_plane = intersect(start_x, start_y);
        running = true;
    }
    void evaluate()
    {
        if (!running) {
            return;
        }
        V delta = intersect(mouse_x, mouse_y) - start_on_plane;
        if (axis >= 0) {
            if (plane) {
                V o, d, so, sd;
                ray(mouse_x, mouse_y, o, d);
                ray(start_x, start_y, so, sd);
                float denominator = dot(d, basis[axis]),
                      start_denominator = dot(sd, basis[axis]);
                if (std::fabs(denominator) > .05f &&
                    std::fabs(start_denominator) > .05f)
                {
                    delta =
                        (o + d * (dot(pivot - o, basis[axis]) / denominator)) -
                        (so + sd * (dot(pivot - so, basis[axis]) /
                                    start_denominator));
                } else {
                    delta = delta - basis[axis] * dot(delta, basis[axis]);
                }
            } else {
                float x, y;
                screen(pivot + basis[axis], x, y);
                float dx = x - pivot_x, dy = y - pivot_y,
                      l2 = dx * dx + dy * dy;
                float distance =
                    l2 > .5f
                        ? ((mouse_x - start_x) * dx +
                           (mouse_y - start_y) * dy) /
                              l2
                        : dot(intersect(start_x, start_y + mouse_y - start_y) -
                                  start_on_plane,
                              unit(cross(view_normal, { 1, 0, 0 })));
                delta = basis[axis] * distance;
            }
        }
        float sx = start_x - pivot_x, sy = start_y - pivot_y,
              ex = mouse_x - pivot_x, ey = mouse_y - pivot_y;
        float initial_radius = std::sqrt(sx * sx + sy * sy);
        float angle = initial_radius > 15
                          ? std::atan2(sx * ey - sy * ex, sx * ex + sy * ey)
                          : (mouse_x - start_x) * .015f;
        float scale = initial_radius > 15
                          ? std::sqrt(ex * ex + ey * ey) / initial_radius
                          : 1 + (mouse_x - start_x) * .01f;
        scale = std::max(.001f, scale);
        M transform = identity();
        if (operation == Rotate) {
            V n = axis >= 0 ? basis[axis] : view_normal;
            // Screen Y grows downwards. Keep clockwise mouse rotation
            // clockwise on screen.
            if (dot(n, view_normal) < 0) {
                angle = -angle;
            }
            float c = std::cos(angle), s = std::sin(angle);
            for (unsigned i = 0; i < 3; ++i) {
                for (unsigned j = 0; j < 3; ++j) {
                    transform.v[i][j] =
                        (i == j ? c : 0) + (1 - c) * n[i] * n[j];
                }
            }
            transform.v[0][1] -= s * n.z;
            transform.v[0][2] += s * n.y;
            transform.v[1][0] += s * n.z;
            transform.v[1][2] -= s * n.x;
            transform.v[2][0] -= s * n.y;
            transform.v[2][1] += s * n.x;
        } else if (operation == Scale) {
            for (unsigned k = 0; k < 3; ++k) {
                if (axis < 0 || (plane ? (int) k != axis : (int) k == axis)) {
                    for (unsigned i = 0; i < 3; ++i) {
                        for (unsigned j = 0; j < 3; ++j) {
                            transform.v[i][j] +=
                                (scale - 1) * basis[k][i] * basis[k][j];
                        }
                    }
                }
            }
        }
        for (unsigned i = 0; i < count; ++i) {
            V center =
                pivot_mode == Individual ? origin(start_world[i]) : pivot;
            V translation =
                operation == Move ? delta : center - point(transform, center);
            for (unsigned a = 0; a < 3; ++a) {
                transform.v[a][3] = translation[a];
            }
            desired_world[i] = multiply(transform, start_world[i]);
            for (unsigned a = 0; a < 3; ++a) {
                transform.v[a][3] = 0;
            }
            nativeHome_WorkshopSetBoneLocal(selected[i], &changes[i].before);
        }
        for (unsigned k = 0; k < count; ++k) {
            unsigned i = apply_order[k];
            M current, local_inverse, parent, parent_inverse;
            nativeHome_WorkshopBoneMatrix(selected[i], current.v);
            if (!inverse(local_matrix(changes[i].before), local_inverse)) {
                continue;
            }
            // Recover the effective parent transform from HSD itself,
            // including its scale compensation flags rather than assuming
            // plain parent * local.
            parent = multiply(current, local_inverse);
            if (!inverse(parent, parent_inverse)) {
                continue;
            }
            changes[i].after = decompose(
                multiply(parent_inverse, desired_world[i]), changes[i].before);
            nativeHome_WorkshopSetBoneLocal(selected[i], &changes[i].after);
        }
    }
    void build_overlay()
    {
        auto element = document->CreateElement("div");
        overlay = element.get();
        overlay->SetClass("workshop-transform-overlay", true);
        std::string html = "<div class='workshop-transform-tools'>";
        for (unsigned i = 0; i < 3; ++i) {
            html += "<button id='transform-tool-" + std::to_string(i) + "'>" +
                    (i == 0   ? "Move"
                     : i == 1 ? "Rotate"
                              : "Scale") +
                    "</button>";
        }
        html += "<select id='transform-pivot'><option value='0'>Selection "
                "center</option><option value='1'>Active bone</option><option "
                "value='2'>Individual origins</option></select><select "
                "id='transform-space'><option value='0'>World</option><option "
                "value='1'>Local</option></select></div>";
        for (unsigned a = 0; a < 3; ++a) {
            for (unsigned segment = 0; segment < 32; ++segment) {
                html += "<button id='transform-axis-" + std::to_string(a) +
                        "-" + std::to_string(segment) +
                        "' class='workshop-transform-axis axis-" +
                        std::to_string(a) + "' title='" +
                        std::string(a == 0   ? "X"
                                    : a == 1 ? "Y"
                                             : "Z") +
                        " axis'></button>";
            }
        }
        html +=
            "<div id='transform-hint' class='workshop-transform-hint'></div>";
        overlay->SetInnerRML(html);
        viewport->AppendChild(std::move(element));
        static_cast<Rml::ElementFormControl*>(
            document->GetElementById("transform-pivot"))
            ->SetValue(std::to_string((int) pivot_mode));
        static_cast<Rml::ElementFormControl*>(
            document->GetElementById("transform-space"))
            ->SetValue(local_axes ? "1" : "0");
        for (unsigned a = 0; a < 3; ++a) {
            tool_buttons[a] = document->GetElementById("transform-tool-" +
                                                       std::to_string(a));
            for (unsigned seg = 0; seg < 32; ++seg) {
                segments[a][seg] = {};
                segments[a][seg].element = document->GetElementById(
                    "transform-axis-" + std::to_string(a) + "-" +
                    std::to_string(seg));
            }
        }
        hint = document->GetElementById("transform-hint");
        drawn_operation = drawn_running = -1;
    }
    void draw_gizmo()
    {
        if (!overlay || !count) {
            return;
        }
        if (!running) {
            axes_and_pivot();
        }
        V center = pivot;
        if (running && operation == Move) {
            // Draw the handles at the translated selection while the immutable
            // original pivot remains the reference for constraint switching.
            V total = {};
            unsigned n = 0;
            for (unsigned i = 0; i < count; ++i) {
                if (pivot_mode == Active && selected[i] != active) {
                    continue;
                }
                M world;
                if (nativeHome_WorkshopBoneMatrix(selected[i], world.v)) {
                    total = total + (origin(world) - origin(start_world[i]));
                    ++n;
                }
            }
            if (n) {
                center = center + total * (1.0f / n);
            }
        }
        float cx, cy;
        if (!screen(center, cx, cy)) {
            return;
        }
        V o, d, o2, d2;
        ray((int) cx, (int) cy, o, d);
        ray((int) cx + 72, (int) cy, o2, d2);
        V normal = unit(d);
        float denominator = dot(d2, normal);
        float size =
            std::fabs(denominator) > 1e-6f
                ? length(o2 + d2 * (dot(center - o2, normal) / denominator) -
                         center)
                : 6;
        auto offset = viewport->GetAbsoluteOffset(Rml::BoxArea::Content);
        for (unsigned a = 0; a < 3; ++a) {
            for (unsigned seg = 0; seg < 32; ++seg) {
                Segment& cached = segments[a][seg];
                auto* e = cached.element;
                bool shown = operation == Rotate || seg == 0;
                V p = center, q = center + basis[a] * size;
                if (operation == Rotate) {
                    float t = seg * 2 * Pi / 32, u = (seg + 1) * 2 * Pi / 32;
                    p = center + (basis[(a + 1) % 3] * std::cos(t) +
                                  basis[(a + 2) % 3] * std::sin(t)) *
                                     size;
                    q = center + (basis[(a + 1) % 3] * std::cos(u) +
                                  basis[(a + 2) % 3] * std::sin(u)) *
                                     size;
                }
                float x, y, xx, yy;
                shown = shown && screen(p, x, y) && screen(q, xx, yy);
                if (!cached.initialized || cached.visible != shown) {
                    e->SetProperty("display", shown ? "block" : "none");
                }
                cached.visible = shown;
                if (shown) {
                    x -= offset.x;
                    y -= offset.y + 2;
                    float width =
                        std::max(4.0f, std::hypot(xx - offset.x - x,
                                                  yy - offset.y - 2 - y));
                    float angle =
                        std::atan2(yy - offset.y - 2 - y, xx - offset.x - x);
                    if (!cached.initialized || cached.x != x) {
                        e->SetProperty("left", std::to_string(x) + "px");
                    }
                    if (!cached.initialized || cached.y != y) {
                        e->SetProperty("top", std::to_string(y) + "px");
                    }
                    if (!cached.initialized || cached.width != width) {
                        e->SetProperty("width", std::to_string(width) + "px");
                    }
                    if (!cached.initialized || cached.angle != angle) {
                        e->SetProperty("transform", "rotate(" +
                                                        std::to_string(angle) +
                                                        "rad)");
                    }
                    bool selected =
                        running && (axis < 0 || (plane ? (int) a != axis
                                                       : (int) a == axis));
                    if (!cached.initialized || cached.selected != selected) {
                        e->SetClass("selected", selected);
                    }
                    cached.x = x;
                    cached.y = y;
                    cached.width = width;
                    cached.angle = angle;
                    cached.selected = selected;
                }
                cached.initialized = true;
            }
        }
        if (drawn_operation != (int) operation) {
            for (unsigned i = 0; i < 3; ++i) {
                tool_buttons[i]->SetClass("selected",
                                          operation == (Operation) i);
            }
            drawn_operation = (int) operation;
        }
        if (drawn_running != (int) running) {
            hint->SetInnerRML(running
                                  ? "LMB / Enter: confirm · RMB / Esc: cancel "
                                    "· X/Y/Z: axis · Shift+X/Y/Z: plane"
                                  : "G: move · R: rotate · S: scale · "
                                    "Shift+click: select bones");
            drawn_running = (int) running;
        }
    }

    class Events final : public Rml::EventListener {
        void ProcessEvent(Rml::Event& e) override
        {
            if (!viewport || !viewport->IsVisible()) {
                return;
            }
            const auto& type = e.GetType();
            if (type == "mousemove" || type == "mousedown") {
                mouse_x = e.GetParameter<int>("mouse_x", mouse_x);
                mouse_y = e.GetParameter<int>("mouse_y", mouse_y);
            }
            const auto id = e.GetTargetElement()->GetId();
            bool consume = false;
            if (type == "mousemove" && running) {
                evaluate();
                consume = true;
            }
            if (type == "mousedown") {
                suppress_click = false;
                int button = e.GetParameter<int>("button", -1);
                if (running && (button == 0 || button == 1)) {
                    finish(button == 0);
                    suppress_click = true;
                    consume = true;
                } else if (button == 0 && id.find("transform-axis-") == 0) {
                    begin(operation, std::atoi(id.c_str() + 15), true);
                    consume = true;
                }
            }
            if (type == "mouseup" && running && gizmo_drag &&
                e.GetParameter<int>("button", -1) == 0)
            {
                finish(true);
                suppress_click = true;
                consume = true;
            }
            if (type == "click") {
                if (suppress_click) {
                    suppress_click = false;
                    consume = true;
                } else if (id.find("transform-tool-") == 0) {
                    operation = (Operation) std::atoi(id.c_str() + 15);
                    consume = true;
                }
            }
            if (type == "change" &&
                (id == "transform-pivot" || id == "transform-space"))
            {
                auto* field = static_cast<Rml::ElementFormControl*>(
                    e.GetTargetElement());
                finish(false);
                int value = std::atoi(field->GetValue().c_str());
                if (id == "transform-pivot") {
                    pivot_mode = (Pivot) value;
                } else {
                    local_axes = value != 0;
                }
                consume = true;
            }
            if (type == "keydown") {
                auto* target = e.GetTargetElement();
                bool typing = false;
                for (; target; target = target->GetParentNode()) {
                    if (target->GetTagName() == "input" ||
                        target->GetTagName() == "textarea" ||
                        target->GetTagName() == "select")
                    {
                        typing = true;
                    }
                }
                int key = e.GetParameter<int>("key_identifier", 0);
                if (running) {
                    if (key == Rml::Input::KI_ESCAPE ||
                        key == Rml::Input::KI_RETURN)
                    {
                        finish(key == Rml::Input::KI_RETURN);
                    } else if (key == Rml::Input::KI_X ||
                               key == Rml::Input::KI_Y ||
                               key == Rml::Input::KI_Z)
                    {
                        int next = key == Rml::Input::KI_X   ? 0
                                   : key == Rml::Input::KI_Y ? 1
                                                             : 2;
                        bool next_plane =
                            e.GetParameter<bool>("shift_key", false);
                        axis = axis == next && plane == next_plane ? -1 : next;
                        plane = next_plane;
                        evaluate();
                    }
                    consume = true;
                } else if (!typing && over_viewport() &&
                           !e.GetParameter<bool>("ctrl_key", false) &&
                           !e.GetParameter<bool>("alt_key", false))
                {
                    if (key == Rml::Input::KI_G || key == Rml::Input::KI_R ||
                        key == Rml::Input::KI_S)
                    {
                        begin(key == Rml::Input::KI_G   ? Move
                              : key == Rml::Input::KI_R ? Rotate
                                                        : Scale,
                              -1, false);
                        consume = true;
                    }
                }
            }
            if (consume) {
                e.StopImmediatePropagation();
            }
        }
    } events;
}

void nativeWorkshopTransform_Init(Rml::ElementDocument* doc)
{
    document = doc;
    for (const char* name :
         { "mousemove", "mousedown", "mouseup", "click", "keydown", "change" })
    {
        document->AddEventListener(name, &events, true);
    }
}
void nativeWorkshopTransform_Clear()
{
    finish(false);
    if (overlay && viewport) {
        viewport->RemoveChild(overlay);
    }
    overlay = nullptr;
    viewport = nullptr;
    count = 0;
    commit = nullptr;
    suppress_click = false;
}
void nativeWorkshopTransform_Update(const char* id, const unsigned* joints,
                                    unsigned n, unsigned active_joint,
                                    uint64_t rev,
                                    NativeWorkshopTransformCommit callback)
{
    n = std::min(n, MaxBones);
    if (!n || !nativeHome_WorkshopCameraActive()) {
        nativeWorkshopTransform_Clear();
        return;
    }
    auto* next = document->GetElementById(id);
    if (next != viewport || revision != rev || active != active_joint ||
        count != n || std::memcmp(selected, joints, n * sizeof(unsigned)))
    {
        finish(false);
        std::memcpy(selected, joints, n * sizeof(unsigned));
        count = n;
        active = active_joint;
        revision = rev;
        if (next != viewport) {
            if (overlay && viewport) {
                viewport->RemoveChild(overlay);
            }
            viewport = next;
            build_overlay();
        }
    }
    commit = callback;
    if (running) {
        for (unsigned i = 0; i < count; ++i) {
            nativeHome_WorkshopSetBoneLocal(selected[i], &changes[i].after);
        }
    }
    draw_gizmo();
}
