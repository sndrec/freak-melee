#pragma once

#include <yyjson.h>
#include <melee/ft/native_authoring_reply.h>
#include <string>
#include <vector>

namespace NativeAgent {
struct Context {
    std::string installation;
    std::string content_root;
    std::string projects_root;
};
using NativeAuthoring::Reply;
using NativeAuthoring::add_string;
using Handler = void (*)(const Context&, yyjson_val*, Reply&);
struct Tool {
    const char* name;
    const char* description;
    const char* schema;
    Handler handler;
    bool read_only;
    bool live = false;
};

bool initialize(Context&, std::string& error);
const Tool* catalog(size_t& count);
yyjson_mut_val* list_tools(yyjson_mut_doc*);
bool call(const Context&, const char* name, yyjson_val* arguments, Reply&);
int mcp(const Context&);
std::string serialize(yyjson_mut_doc*);
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
void build_project(const Context&, yyjson_val*, Reply&);
void export_package(const Context&, yyjson_val*, Reply&);
void update_scripts(const Context&, yyjson_val*, Reply&);
void parameter_source(const Context&, yyjson_val*, Reply&);
void attributes(const Context&, yyjson_val*, Reply&);
void edit_attributes(const Context&, yyjson_val*, Reply&);
void actions(const Context&, yyjson_val*, Reply&);
void read_actions(const Context&, yyjson_val*, Reply&);
void edit_actions(const Context&, yyjson_val*, Reply&);
void read_animations(const Context&, yyjson_val*, Reply&);
void edit_animations(const Context&, yyjson_val*, Reply&);
void documentation(const Context&, yyjson_val*, Reply&);
void documentation_page(const Context&, yyjson_val*, Reply&);
void examples(const Context&, yyjson_val*, Reply&);
void example(const Context&, yyjson_val*, Reply&);
void running_instances(const Context&, yyjson_val*, Reply&);
void live_command(const char* name, yyjson_val*, Reply&);
}
