#pragma once

#include <yyjson.h>
#include <initializer_list>

namespace NativeAgent {
struct Tool;
namespace ResultSchema {
struct Field {
    const char* name;
    const char* type;
};

struct Object {
    yyjson_mut_doc* document;
    yyjson_mut_val* schema;
    yyjson_mut_val* properties;
    yyjson_mut_val* required;
    Object(yyjson_mut_doc*, bool closed = true);
    Object& fields(std::initializer_list<Field>, bool mandatory = true);
    Object& add(const char*, yyjson_mut_val*, bool mandatory = true);
    Object& merge(const Object&);
};

struct Catalog {
    yyjson_mut_doc* document;
    yyjson_mut_val* entries;
    explicit Catalog(yyjson_mut_doc*);
    Object object(std::initializer_list<Field> fields = {});
    yyjson_mut_val* type(const char*);
    yyjson_mut_val* nullable(const char*);
    yyjson_mut_val* map(yyjson_mut_val* values);
    yyjson_mut_val* array(yyjson_mut_val* items);
    yyjson_mut_val* array(const char* item_type);
    yyjson_mut_val* choice(std::initializer_list<yyjson_mut_val*>);
    yyjson_mut_val* enumeration(std::initializer_list<const char*>);
    void add(std::initializer_list<const char*> names, const Object&);
    void add(const char* name, yyjson_mut_val* schema);
    yyjson_mut_val* for_tool(const Tool&);
};

Object saved_draft(Catalog&);
Object source_status(Catalog&);
Object script_validation(Catalog&);
Object workshop_state(Catalog&);
Object workshop_mutation(Catalog&);
void add_core(Catalog&);
void add_editing(Catalog&);
void add_sessions(Catalog&);
}
}
