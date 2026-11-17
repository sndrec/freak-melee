#pragma once

#include <yyjson.h>
#include <string>
#include <vector>

namespace NativeAgent {
struct Context {
    std::string installation;
    std::string content_root;
    std::string projects_root;
};
struct Reply {
    yyjson_mut_doc* document;
    yyjson_mut_val* result;
    bool ok = true;
    explicit Reply(yyjson_mut_doc* owner);
    void fail(const char* code, const std::string& message);
};
using Handler = void (*)(const Context&, yyjson_val*, Reply&);
struct Tool {
    const char* name;
    const char* description;
    const char* schema;
    Handler handler;
    bool read_only;
};

bool initialize(Context&, std::string& error);
const Tool* catalog(size_t& count);
yyjson_mut_val* list_tools(yyjson_mut_doc*);
bool call(const Context&, const char* name, yyjson_val* arguments, Reply&);
int mcp(const Context&);
std::string serialize(yyjson_mut_doc*);
void add_string(Reply&, yyjson_mut_val*, const char*, const std::string&);
const char* string(yyjson_val*, const char*);
unsigned page_offset(yyjson_val*);
unsigned page_limit(yyjson_val*);
bool read_text(const std::string& path, std::string&, size_t maximum);
bool matches(const std::string& text, const std::string& query);
std::vector<std::string> documentation_pages(const Context&);
bool read_document(const Context&, const std::string& page, std::string& text);
std::vector<std::string> example_names(const Context&);
bool read_example(const Context&, const std::string& name, std::string& text);

void environment(const Context&, yyjson_val*, Reply&);
void projects(const Context&, yyjson_val*, Reply&);
void project(const Context&, yyjson_val*, Reply&);
void sources(const Context&, yyjson_val*, Reply&);
void source(const Context&, yyjson_val*, Reply&);
void check_scripts(const Context&, yyjson_val*, Reply&);
void parameter_source(const Context&, yyjson_val*, Reply&);
void attributes(const Context&, yyjson_val*, Reply&);
void actions(const Context&, yyjson_val*, Reply&);
void documentation(const Context&, yyjson_val*, Reply&);
void documentation_page(const Context&, yyjson_val*, Reply&);
void examples(const Context&, yyjson_val*, Reply&);
void example(const Context&, yyjson_val*, Reply&);
}
