#include "native_shell_workshop_code.h"
#ifdef MELEE_NATIVE
#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/ElementUtilities.h>
#include <RmlUi/Core/Elements/ElementFormControlTextArea.h>
#include <RmlUi/Core/FontEngineInterface.h>
#include <RmlUi/Core/FontMetrics.h>
#include <RmlUi/Core/Geometry.h>
#include <RmlUi/Core/RenderManager.h>
#include <RmlUi/Core/TextShapingContext.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

namespace {
struct Token { size_t begin, end; Rml::Colourb colour; };
struct Run { float left, right; Rml::Colourb colour; };
struct Draw { Rml::Geometry geometry; Rml::Texture texture; };
bool identifier(unsigned char c) { return std::isalnum(c) || c == '_' || c >= 128; }
std::vector<Token> tokenize(const std::string& line)
{
    std::vector<Token> result;
    static const char* keywords[] = { "fighter", "move", "action", "func", "None", "article", "item", "effect", "on", "state", "parameter", "function", "export", "let", "if", "elif", "else", "while", "return", "break", "continue", "pass", "and", "or", "not", "true", "false", "float", "int", "bool", "vector", "behavior", "independent" };
    for (size_t i = 0; i < line.size();) {
        if (std::isspace((unsigned char)line[i])) { ++i; continue; }
        const size_t start = i;
        Rml::Colourb colour(224, 225, 231);
        if (line[i] == '#') { i = line.size(); colour = { 126, 151, 137 }; }
        else if (line[i] == '"' || line[i] == '\'') {
            const char quote = line[i++];
            while (i < line.size()) {
                if (line[i] == '\\' && i + 1 < line.size()) { i += 2; continue; }
                if (line[i++] == quote) break;
            }
            colour = { 221, 187, 133 };
        } else if (std::isdigit((unsigned char)line[i])) {
            while (i < line.size() && (identifier((unsigned char)line[i]) || line[i] == '.')) ++i;
            colour = { 244, 166, 127 };
        } else if (identifier((unsigned char)line[i])) {
            while (i < line.size() && identifier((unsigned char)line[i])) ++i;
            const std::string word = line.substr(start, i - start);
            for (const char* keyword : keywords)
                if (word == keyword) { colour = { 192, 155, 226 }; break; }
            size_t after = i;
            while (after < line.size() && std::isspace((unsigned char)line[after])) ++after;
            if (after < line.size() && line[after] == '(') colour = { 144, 205, 206 };
            else if (after < line.size() && line[after] == '.') colour = { 159, 192, 229 };
        } else { ++i; colour = { 165, 170, 186 }; }
        result.push_back({ start, i, colour });
    }
    return result;
}

class CodeColours final : public Rml::Element {
public:
    explicit CodeColours(const Rml::String& tag) : Element(tag) {}
private:
    std::string cached_source;
    Rml::Vector2f cached_origin, cached_size;
    Rml::FontFaceHandle cached_font = 0;
    int cached_font_version = -1, cached_start = -1, cached_end = -1;
    std::vector<Draw> draws;
    void OnRender() override
    {
        auto* document = GetOwnerDocument();
        if (!document) return;
        auto* source = static_cast<Rml::ElementFormControlTextArea*>(document->GetElementById("workshop-script-source"));
        if (!source || !source->GetFontFaceHandle()) return;
        auto* engine = Rml::GetFontEngineInterface();
        const auto font = source->GetFontFaceHandle();
        const int version = engine->GetVersion(font);
        const auto absolute = GetAbsoluteOffset();
        const Rml::Vector2f origin = source->GetAbsoluteOffset(Rml::BoxArea::Content) - absolute -
            Rml::Vector2f(source->GetScrollLeft(), source->GetScrollTop());
        const auto size = GetBox().GetSize();
        int selection_start, selection_end;
        Rml::String selected;
        source->GetSelection(&selection_start, &selection_end, &selected);
        const std::string text = source->GetValue();
        if (text != cached_source || origin != cached_origin || size != cached_size ||
            font != cached_font || version != cached_font_version ||
            selection_start != cached_start || selection_end != cached_end) {
            cached_source = text; cached_origin = origin; cached_size = size;
            cached_font = font; cached_font_version = version;
            cached_start = selection_start; cached_end = selection_end;
            draws.clear();
            auto* manager = GetRenderManager();
            if (!manager) return;
            const auto& computed = source->GetComputedValues();
            const Rml::TextShapingContext shaping { computed.language(), computed.direction(), computed.font_kerning(), computed.letter_spacing() };
            const auto& metrics = engine->GetFontMetrics(font);
            // The text input rounds its line height and adds the same half-leading.
            const float height = std::round(source->GetLineHeight());
            const float baseline = metrics.ascent + 0.5f * (height - metrics.ascent - metrics.descent);
            float y = origin.y + baseline;
            size_t start = 0;
            for (; start < text.size(); y += height) {
                const size_t newline = text.find('\n', start);
                const size_t end = newline == std::string::npos ? text.size() : newline;
                if (y + metrics.descent >= 0 && y - metrics.ascent <= size.y) {
                    const std::string line = text.substr(start, end - start);
                    std::vector<Run> runs;
                    for (const auto& token : tokenize(line)) {
                        runs.push_back({
                            float(engine->GetStringWidth(font, Rml::StringView(line.data(), line.data() + token.begin), shaping)),
                            float(engine->GetStringWidth(font, Rml::StringView(line.data(), line.data() + token.end), shaping)), token.colour });
                    }
                    Rml::TexturedMeshList meshes;
                    engine->GenerateString(*manager, font, 0, line, { origin.x, y },
                        Rml::Colourb(224, 225, 231).ToPremultiplied(), 1.0f, shaping, meshes);
                    for (auto& textured : meshes) {
                        auto& vertices = textured.mesh.vertices;
                        for (size_t quad = 0; quad + 3 < vertices.size(); quad += 4) {
                            const float x = (vertices[quad].position.x + vertices[quad + 2].position.x) * 0.5f - origin.x;
                            Rml::Colourb colour(224, 225, 231);
                            for (const auto& run : runs)
                                if (x >= run.left && x < run.right) { colour = run.colour; break; }
                            // Selected text is drawn by the real text input above this layer.
                            const size_t left = std::clamp<size_t>(selection_start < 0 ? 0 : selection_start, start, end) - start;
                            const size_t right = std::clamp<size_t>(selection_end < 0 ? 0 : selection_end, start, end) - start;
                            if (selection_end > selection_start && x >= engine->GetStringWidth(font, Rml::StringView(line.data(), line.data() + left), shaping) &&
                                x < engine->GetStringWidth(font, Rml::StringView(line.data(), line.data() + right), shaping)) colour.alpha = 0;
                            const auto premultiplied = colour.ToPremultiplied();
                            for (size_t v = 0; v < 4; ++v) vertices[quad + v].colour = premultiplied;
                        }
                        draws.push_back({ manager->MakeGeometry(std::move(textured.mesh)), textured.texture });
                    }
                }
                if (newline == std::string::npos) break;
                start = newline + 1;
                if (y > size.y + height) break;
            }
        }
        Rml::ElementUtilities::SetClippingRegion(this);
        auto* manager=GetRenderManager();
        const auto previous=manager->GetScissorRegion();
        const auto content=source->GetAbsoluteOffset(Rml::BoxArea::Content);
        const auto bounds=source->GetBox().GetSize(Rml::BoxArea::Content);
        const auto clip=Rml::Rectanglei::FromPositionSize(
            {int(content.x),int(content.y)},{int(bounds.x),int(bounds.y)});
        manager->SetScissorRegion(clip.IntersectIfValid(previous));
        for (const auto& draw : draws) draw.geometry.Render(absolute, draw.texture);
        manager->SetScissorRegion(previous);
    }
};
Rml::ElementInstancerGeneric<CodeColours> instancer;
}

void nativeWorkshopCode_Register()
{
    Rml::Factory::RegisterElementInstancer("script-colours", &instancer);
}
#endif
