#include "native_shell_workshop_docs.h"
#ifdef MELEE_NATIVE
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlTextArea.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/FontEngineInterface.h>
#include <RmlUi/Core/TextShapingContext.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
struct Page { std::string file, title, text; bool source = false; };
std::vector<Page> pages;
Rml::ElementDocument* document;
std::string symbol;
bool visible, menu_visible;
std::string escape(const std::string& s) { return Rml::StringUtilities::EncodeRml(s); }
std::string lower(std::string s) { for (char& c : s) c = (char)std::tolower((unsigned char)c); return s; }
std::string inline_text(const std::string& text)
{
    std::string out;
    for (size_t i=0; i<text.size();) {
        if (text[i]=='[') {
            size_t end=text.find("](",i), close=end==std::string::npos?end:text.find(')',end+2);
            if (close!=std::string::npos) {
                std::string file=text.substr(end+2,close-end-2);
                const size_t hash=file.find('#'); if(hash!=std::string::npos)file.resize(hash);
                for(size_t p=0;p<pages.size();++p) if(pages[p].file==file) {
                    out+="<button class='doc-link' id='workshop-doc-page-"+std::to_string(p)+"'>"+escape(text.substr(i+1,end-i-1))+"</button>";
                    i=close+1; break;
                }
                if(i==close+1)continue;
                out+=inline_text(text.substr(i+1,end-i-1));i=close+1;continue;
            }
        }
        if(text[i]=='`') { size_t end=text.find('`',i+1); if(end!=std::string::npos) {
            out+="<code>"+escape(text.substr(i+1,end-i-1))+"</code>"; i=end+1;continue;
        }}
        if(i+1<text.size()&&text[i]=='*'&&text[i+1]=='*') {size_t end=text.find("**",i+2);if(end!=std::string::npos){
            out+="<strong>"+escape(text.substr(i+2,end-i-2))+"</strong>";i=end+2;continue;
        }}
        out+=escape(text.substr(i++,1));
    }
    return out;
}
std::vector<std::string> cells(const std::string& line)
{
    std::vector<std::string> result;
    size_t start = line.empty() || line[0] != '|' ? 0 : 1;
    while (start < line.size()) {
        const size_t end = line.find('|', start);
        std::string cell = line.substr(start, end == std::string::npos ? end : end - start);
        const size_t first = cell.find_first_not_of(" ");
        if (first == std::string::npos) cell.clear();
        else cell = cell.substr(first, cell.find_last_not_of(" ") - first + 1);
        result.push_back(std::move(cell));
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return result;
}
void open_page(size_t index)
{
    if (index >= pages.size()) return;
    const auto& page = pages[index];
    if (page.source) {
        document->GetElementById("workshop-doc-content")->SetInnerRML("<pre><code>" + escape(page.text) + "</code></pre>");
        document->GetElementById("workshop-doc-title")->SetInnerRML(escape(page.title));
        document->GetElementById("workshop-doc-panel")->SetProperty("display", "block");
        visible = true;
        return;
    }
    std::vector<std::string> lines;
    size_t pos = 0;
    while (pos < page.text.size()) {
        size_t end = page.text.find('\n', pos);
        if (end == std::string::npos) end = page.text.size();
        std::string line = page.text.substr(pos, end - pos);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(std::move(line));
        pos = end + 1;
    }
    std::string markup, paragraph;
    bool code = false, list = false;
    auto flush = [&]() {
        if (!paragraph.empty()) { markup += "<p>" + inline_text(paragraph) + "</p>"; paragraph.clear(); }
    };
    auto finish_list = [&]() { if (list) { markup += "</ul>"; list = false; } };
    for (size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        if (line.compare(0, 3, "```") == 0) {
            flush(); finish_list(); markup += code ? "</pre>" : "<pre>"; code = !code; continue;
        }
        if (code) { markup += escape(line) + "\n"; continue; }
        if (line.empty()) { flush(); finish_list(); continue; }
        size_t heading = 0;
        while (heading < line.size() && line[heading] == '#') ++heading;
        if (heading && heading < line.size() && line[heading] == ' ') {
            flush(); finish_list();
            const std::string tag = heading == 1 ? "h1" : heading == 2 ? "h2" : "h3";
            markup += "<" + tag + ">" + inline_text(line.substr(heading + 1)) + "</" + tag + ">";
        } else if (line[0] == '|' && i + 1 < lines.size() && lines[i + 1].find("---") != std::string::npos) {
            flush(); finish_list(); markup += "<table><tr>";
            for (const auto& cell : cells(line)) markup += "<th>" + inline_text(cell) + "</th>";
            markup += "</tr>"; ++i;
            while (i + 1 < lines.size() && !lines[i + 1].empty() && lines[i + 1][0] == '|') {
                markup += "<tr>";
                for (const auto& cell : cells(lines[++i])) markup += "<td>" + inline_text(cell) + "</td>";
                markup += "</tr>";
            }
            markup += "</table>";
        } else if (line.compare(0, 2, "- ") == 0 || line.compare(0, 2, "* ") == 0) {
            flush(); if (!list) { markup += "<ul>"; list = true; }
            std::string item = line.substr(2);
            while (i + 1 < lines.size() && lines[i + 1].compare(0, 2, "  ") == 0)
                item += " " + lines[++i].substr(2);
            markup += "<li>• " + inline_text(item) + "</li>";
        } else {
            finish_list(); if (!paragraph.empty()) paragraph += ' '; paragraph += line;
        }
    }
    flush(); finish_list(); if (code) markup += "</pre>";
    document->GetElementById("workshop-doc-content")->SetInnerRML(markup);
    document->GetElementById("workshop-doc-content")->SetScrollTop(0);
    document->GetElementById("workshop-doc-title")->SetInnerRML(escape(page.title));
    document->GetElementById("workshop-doc-panel")->SetProperty("display", "block"); visible = true;
}
void search(const std::string& query)
{
    const std::string needle=lower(query);std::string markup;size_t best=pages.size();
    for(size_t i=0;i<pages.size();++i) {
        if(!needle.empty()&&lower(pages[i].text).find(needle)==std::string::npos)continue;
        markup+="<button class='doc-page' id='workshop-doc-page-"+std::to_string(i)+"'>"+escape(pages[i].title)+"</button>";
        if(best==pages.size())best=i;
        if(!needle.empty()&&pages[i].text.find("`"+query+"(")!=std::string::npos)best=i;
    }
    document->GetElementById("workshop-doc-results")->SetInnerRML(markup.empty()?"<p>No matching reference page.</p>":markup);
    if(query=="command")for(size_t i=0;i<pages.size();++i)if(pages[i].file=="command_variables.md")best=i;
    if(needle.empty())for(size_t i=0;i<pages.size();++i)if(pages[i].file=="index.md")best=i;
    if(best<pages.size())open_page(best);
    else {
        document->GetElementById("workshop-doc-content")->SetInnerRML("<p>No matching reference page. Try a function name or topic.</p>");
        document->GetElementById("workshop-doc-title")->SetInnerRML("Search reference");
        document->GetElementById("workshop-doc-panel")->SetProperty("display","block");visible=true;
    }
}
std::string symbol_at(Rml::ElementFormControlTextArea* source, float x, float y)
{
    const std::string text=source->GetValue();
    const auto origin=source->GetAbsoluteOffset(Rml::BoxArea::Content);
    const int row=std::max(0,(int)std::floor((y-origin.y+source->GetScrollTop())/std::round(source->GetLineHeight())));
    size_t start=0;for(int i=0;i<row;++i){start=text.find('\n',start);if(start==std::string::npos)return {}; ++start;}
    size_t end=text.find('\n',start);if(end==std::string::npos)end=text.size();
    const auto& cv=source->GetComputedValues();
    const Rml::TextShapingContext shaping{cv.language(),cv.direction(),cv.font_kerning(),cv.letter_spacing()};
    auto* font=Rml::GetFontEngineInterface();size_t at=start;
    for(;at<end;++at)if(font->GetStringWidth(source->GetFontFaceHandle(),Rml::StringView(text.data()+start,text.data()+at+1),shaping)>x-origin.x+source->GetScrollLeft())break;
    auto word=[](char c){return std::isalnum((unsigned char)c)||c=='_'||c=='.';};
    if(at==end&&at>start)--at;if(at>=text.size()||!word(text[at]))return {};
    size_t left=at,right=at+1;while(left>start&&word(text[left-1]))--left;while(right<end&&word(text[right]))++right;
    return text.substr(left,right-left);
}
class Events final:public Rml::EventListener {
    void ProcessEvent(Rml::Event& event) override {
        auto* target=event.GetTargetElement();while(target&&target->GetId().empty())target=target->GetParentNode();if(!target)return;
        const std::string id=target->GetId();
        if(event.GetType()=="mousedown"&&id=="workshop-script-source"&&event.GetParameter<int>("button",0)==1) {
            float x=event.GetParameter<float>("mouse_x",0),y=event.GetParameter<float>("mouse_y",0);
            symbol=symbol_at(static_cast<Rml::ElementFormControlTextArea*>(target),x,y);
            if(symbol.empty())return;
            auto* menu=document->GetElementById("workshop-doc-menu");
            menu->SetInnerRML("<button id='workshop-doc-lookup'>Lookup "+escape(symbol)+"</button>");
            const auto size=document->GetContext()->GetDimensions();
            menu->SetProperty("left",std::to_string(std::min(x,(float)size.x-280))+"px");menu->SetProperty("top",std::to_string(std::min(y,(float)size.y-60))+"px");
            menu->SetProperty("display","block");menu_visible=true;event.StopPropagation();
        } else if(event.GetType()=="click") {
            document->GetElementById("workshop-doc-menu")->SetProperty("display","none");menu_visible=false;
            if(id=="workshop-doc-lookup") {static_cast<Rml::ElementFormControlInput*>(document->GetElementById("workshop-doc-search"))->SetValue(symbol);search(symbol);}
            else if(id=="workshop-doc-open")search("");
            else if(id=="workshop-doc-close")nativeWorkshopDocs_Close();
            else if(id.compare(0,18,"workshop-doc-page-")==0)open_page(std::strtoul(id.c_str()+18,nullptr,10));
        } else if(event.GetType()=="change"&&id=="workshop-doc-search")search(static_cast<Rml::ElementFormControlInput*>(target)->GetValue());
    }
} events;
}
void nativeWorkshopDocs_Init(Rml::ElementDocument* shell)
{
    document=shell;wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);
    const auto root=std::filesystem::path(exe).parent_path()/"ui"/"docs";
    std::error_code error;
    for(const auto& entry:std::filesystem::directory_iterator(root,error)) {
        if(entry.path().extension()!=L".md"||entry.file_size(error)>512*1024)continue;
        std::ifstream stream(entry.path(),std::ios::binary);std::string text((std::istreambuf_iterator<char>(stream)),{});
        std::string title=entry.path().stem().string();size_t heading=text.find("# ");if(heading!=std::string::npos){size_t end=text.find('\n',heading);title=text.substr(heading+2,end-heading-2);}
        pages.push_back({entry.path().filename().string(),title,std::move(text)});
    }
    for (const auto& entry : std::filesystem::directory_iterator(root.parent_path(), error)) {
        if (entry.path().extension() != L".fms" || entry.file_size(error) > 512 * 1024) continue;
        std::ifstream stream(entry.path(), std::ios::binary);
        std::string text((std::istreambuf_iterator<char>(stream)), {});
        const std::string name = entry.path().filename().string();
        pages.push_back({"../" + name, name, std::move(text), true});
    }
    std::sort(pages.begin(),pages.end(),[](const Page&a,const Page&b){return a.title<b.title;});
    shell->AddEventListener("mousedown",&events,true);
    shell->AddEventListener("click",&events);shell->GetElementById("workshop-doc-search")->AddEventListener("change",&events);
}
bool nativeWorkshopDocs_Close()
{
    if(menu_visible){document->GetElementById("workshop-doc-menu")->SetProperty("display","none");menu_visible=false;return true;}
    if(visible){document->GetElementById("workshop-doc-panel")->SetProperty("display","none");visible=false;return true;}return false;
}
#endif
