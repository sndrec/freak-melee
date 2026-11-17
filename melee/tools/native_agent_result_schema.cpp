#include "native_agent_result_schema.h"
#include "native_agent_internal.h"
#include <cstring>

namespace NativeAgent::ResultSchema {
Object::Object(yyjson_mut_doc* doc, bool closed)
    : document(doc), schema(yyjson_mut_obj(doc)), properties(yyjson_mut_obj(doc)),
      required(yyjson_mut_arr(doc))
{
    yyjson_mut_obj_add_str(doc, schema, "type", "object");
    yyjson_mut_obj_add_val(doc, schema, "properties", properties);
    yyjson_mut_obj_add_val(doc, schema, "required", required);
    yyjson_mut_obj_add_bool(doc, schema, "additionalProperties", !closed);
}

Object& Object::fields(std::initializer_list<Field> fields, bool mandatory)
{
    for (const auto& field : fields) {
        auto* property = yyjson_mut_obj(document);
        yyjson_mut_obj_add_str(document, property, "type", field.type);
        add(field.name, property, mandatory);
    }
    return *this;
}

Object& Object::add(const char* name, yyjson_mut_val* property, bool mandatory)
{
    yyjson_mut_obj_put(properties, yyjson_mut_str(document, name), property);
    if (mandatory) {
        size_t index, count;
        yyjson_mut_val* field;
        yyjson_mut_arr_foreach(required, index, count, field)
            if (!std::strcmp(yyjson_mut_get_str(field), name)) return *this;
        yyjson_mut_arr_append(required, yyjson_mut_str(document, name));
    }
    return *this;
}

Object& Object::merge(const Object& other)
{
    size_t index, count;
    yyjson_mut_val *key, *value;
    yyjson_mut_obj_foreach(other.properties, index, count, key, value) {
        const char* name = yyjson_mut_get_str(key);
        size_t position, length;
        yyjson_mut_val* field;
        bool mandatory = false;
        yyjson_mut_arr_foreach(other.required, position, length, field)
            if (!std::strcmp(yyjson_mut_get_str(field), name)) { mandatory = true; break; }
        add(name, yyjson_mut_val_mut_copy(document, value), mandatory);
    }
    return *this;
}

Catalog::Catalog(yyjson_mut_doc* doc) : document(doc), entries(yyjson_mut_obj(doc))
{
    add_core(*this);
    add_editing(*this);
    add_sessions(*this);
}

Object Catalog::object(std::initializer_list<Field> fields)
{
    Object result(document);
    result.fields(fields);
    return result;
}

yyjson_mut_val* Catalog::type(const char* name)
{
    auto* schema = yyjson_mut_obj(document);
    yyjson_mut_obj_add_str(document, schema, "type", name);
    return schema;
}

yyjson_mut_val* Catalog::nullable(const char* name)
{
    auto* schema = yyjson_mut_obj(document);
    auto* types = yyjson_mut_arr(document);
    yyjson_mut_arr_append(types, yyjson_mut_str(document, name));
    yyjson_mut_arr_append(types, yyjson_mut_str(document, "null"));
    yyjson_mut_obj_add_val(document, schema, "type", types);
    return schema;
}

yyjson_mut_val* Catalog::array(yyjson_mut_val* items)
{
    auto* schema = type("array");
    yyjson_mut_obj_add_val(document, schema, "items", items);
    return schema;
}

yyjson_mut_val* Catalog::map(yyjson_mut_val* values)
{
    auto* schema = type("object");
    yyjson_mut_obj_add_val(document, schema, "additionalProperties", values);
    return schema;
}

yyjson_mut_val* Catalog::array(const char* item_type) { return array(type(item_type)); }

yyjson_mut_val* Catalog::choice(std::initializer_list<yyjson_mut_val*> alternatives)
{
    auto* schema = yyjson_mut_obj(document);
    auto* variants = yyjson_mut_arr(document);
    for (auto* variant : alternatives) yyjson_mut_arr_append(variants, variant);
    yyjson_mut_obj_add_val(document, schema, "anyOf", variants);
    return schema;
}

yyjson_mut_val* Catalog::enumeration(std::initializer_list<const char*> values)
{
    auto* schema = type("string");
    auto* entries = yyjson_mut_arr(document);
    for (const char* value : values) yyjson_mut_arr_append(entries, yyjson_mut_str(document, value));
    yyjson_mut_obj_add_val(document, schema, "enum", entries);
    return schema;
}

void Catalog::add(std::initializer_list<const char*> names, const Object& result)
{
    for (const char* name : names)
        add(name, result.schema);
}

void Catalog::add(const char* name, yyjson_mut_val* schema)
{
    yyjson_mut_obj_add_val(document, entries, name, yyjson_mut_val_mut_copy(document, schema));
}

yyjson_mut_val* Catalog::for_tool(const Tool& tool)
{
    auto* success = yyjson_mut_obj_get(entries, tool.name);
    if (success) success = yyjson_mut_val_mut_copy(document, success);
    else if (tool.output_schema) {
        auto* parsed = yyjson_read(tool.output_schema, std::strlen(tool.output_schema), 0);
        success = yyjson_val_mut_copy(document, yyjson_doc_get_root(parsed));
        yyjson_doc_free(parsed);
    }
    if (!success) return nullptr;
    if (tool.live) {
        auto* properties = yyjson_mut_obj_get(success, "properties");
        if (properties && !yyjson_mut_obj_get(properties, "request_id"))
            yyjson_mut_obj_add_val(document, properties, "request_id", type("string"));
    }
    Object error(document, false);
    error.fields({{"code", "string"}, {"message", "string"}});
    error.fields({{"request_id", "string"}, {"revision", "string"}}, false);
    auto* schema = choice({success, error.schema});
    yyjson_mut_obj_add_str(document, schema, "type", "object");
    return schema;
}

Object saved_draft(Catalog& c)
{
    return c.object({{"member", "string"}, {"member_path", "string"}, {"revision", "string"}})
        .add("draft_view", c.enumeration({"saved_files"}));
}
}
