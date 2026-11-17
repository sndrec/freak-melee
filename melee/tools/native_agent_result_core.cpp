#include "native_agent_result_schema.h"

namespace NativeAgent::ResultSchema {
namespace {
Object paged(Catalog& c, const char* key, Object row)
{
    return c.object({{"total", "integer"}})
        .add("next_offset", c.nullable("integer"))
        .add(key, c.array(row.schema));
}
}

Object source_status(Catalog& c)
{
    return c.object({{"editable_source_sha256", "string"}, {"parameter_sha256", "string"},
                     {"combined_bytes", "integer"}, {"matches_validated_source", "boolean"},
                     {"imported_parameters", "boolean"}})
        .add("source_origin", c.enumeration({"validated_source", "saved_draft", "stock_template", "separate_files"}))
        .fields({{"parameter_import_error", "string"}}, false);
}

Object script_diagnostic(Catalog& c)
{
    return c.object({{"severity", "string"}, {"message", "string"}})
        .add("file", c.nullable("string"), false)
        .add("line", c.nullable("integer"))
        .add("column", c.nullable("integer"), false)
        .fields({{"phase", "string"}, {"source_view", "string"}}, false);
}

Object script_validation(Catalog& c)
{
    return c.object({{"build_identity", "string"}, {"accepted", "boolean"}, {"diagnostic_count", "integer"},
                 {"metadata_complete", "boolean"}, {"bodies_complete", "boolean"}})
        .fields({{"functions_checked", "integer"}, {"diagnostics_truncated", "boolean"},
                 {"package_diagnostic", "boolean"}}, false)
        .add("diagnostics", c.array(script_diagnostic(c).schema))
        .add("statistics", c.object({{"functions", "integer"}, {"instructions", "integer"},
                                    {"parameters", "integer"}}).schema, false);
}

void add_core(Catalog& c)
{
    c.add({"environment.describe"},
        c.object({{"installation", "string"}, {"content_root", "string"}, {"projects_root", "string"},
                  {"build", "string"}, {"source_revision", "string"}, {"authoring_build_identity", "string"},
                  {"documentation_available", "boolean"}, {"script_source_capacity_bytes", "integer"},
                  {"family_member_capacity", "integer"}, {"live_session_tools_available", "boolean"},
                  {"edit_tools_available", "boolean"}})
            .add("tools", c.array("string"))
            .add("configured_iso", c.nullable("string"))
            .fields({{"configured_iso_available", "boolean"}}, false));

    c.add({"instances.list"},
        c.object({{"build_identity", "string"}})
            .add("instances", c.array(c.object({{"instance", "string"}, {"process_id", "integer"}, {"available", "boolean"}})
                .fields({{"build_identity", "string"}, {"source_revision", "string"}, {"build", "string"},
                         {"executable", "string"}, {"build_matches", "boolean"}, {"connection_status", "string"}}, false).schema)));
    c.add({"stock.list"}, paged(c, "fighters",
        c.object({{"fighter_id", "integer"}, {"name", "string"}, {"member_count", "integer"},
                  {"primary", "string"}, {"template_available", "boolean"}})
            .add("mode", c.enumeration({"single", "alternate", "companion"}))
            .fields({{"partner", "string"}}, false)));
    c.add({"fighters.installed"}, paged(c, "fighters",
        c.object({{"package_id", "string"}, {"title", "string"}, {"tile_name", "string"}, {"member_count", "integer"}}))
        .fields({{"content_root", "string"}}));
    c.add({"projects.list"}, paged(c, "projects", c.object({{"title", "string"}, {"project", "string"}})));
    c.add({"docs.search"}, paged(c, "pages", c.object({{"page", "string"}, {"uri", "string"}, {"excerpt", "string"}})));
    c.add({"docs.read"}, c.object({{"page", "string"}, {"uri", "string"}, {"text", "string"}}));
    c.add({"examples.list"}, paged(c, "examples", c.object({{"example", "string"}, {"uri", "string"}})));
    c.add({"examples.read"}, c.object({{"example", "string"}, {"uri", "string"}, {"text", "string"}}));

    c.add({"project.clone_stock", "project.clone_installed", "project.import"},
        c.object({{"project", "string"}, {"title", "string"}, {"revision", "string"}, {"agent_guide", "string"},
                  {"saved", "boolean"}, {"installed", "boolean"}})
            .add("status", c.enumeration({"created_draft"}))
            .add("members", c.array(c.object({{"key", "string"}, {"path", "string"}}).schema)));
    c.add({"project.rename"},
        c.object({{"project", "string"}, {"title", "string"}, {"revision", "string"}, {"changed", "boolean"},
                  {"saved", "boolean"}, {"installed", "boolean"}})
            .add("status", c.enumeration({"saved_draft"})));
    c.add({"project.inspect"}, saved_draft(c).merge(source_status(c))
        .add("family_mode", c.enumeration({"single", "alternate_form", "simultaneous_companion", "unknown"}))
        .add("members", c.array(c.object({{"key", "string"}, {"path", "string"}, {"title", "string"},
                {"actions", "integer"}, {"demos", "integer"}, {"fighter_bytes", "integer"},
                {"animation_bytes", "integer"}, {"costume_bytes", "integer"}, {"resource_bytes", "integer"},
                {"joints", "integer"}, {"part_slots", "integer"}, {"has_validated_source", "boolean"}}).schema)));
    c.add({"scripts.list"}, saved_draft(c).merge(source_status(c))
        .add("files", c.array(c.object({{"name", "string"}, {"file", "string"}, {"path", "string"},
                                       {"bytes", "integer"}, {"saved_as_separate_file", "boolean"}}).schema)));
    c.add({"scripts.read"}, saved_draft(c).merge(source_status(c)).fields({{"file", "string"}, {"text", "string"}}));
    auto source_check = saved_draft(c).merge(source_status(c)).merge(script_validation(c));
    c.add({"scripts.check"}, source_check);
    c.add({"scripts.update"}, saved_draft(c).merge(source_status(c))
        .add("revision", c.nullable("string"))
        .add("status", c.enumeration({"saved_draft"}))
        .fields({{"previous_revision", "string"}, {"committed", "boolean"}, {"edited_files", "integer"},
                 {"validated", "boolean"}, {"installed", "boolean"}})
        .fields({{"revision_diagnostic", "string"}}, false));

    auto member_error = c.object({{"key", "string"}, {"code", "string"}, {"message", "string"}});
    auto member_checked = source_check.fields({{"key", "string"}});
    auto build = saved_draft(c)
        .fields({{"build_identity", "string"}, {"accepted", "boolean"}, {"diagnostic_count", "integer"}})
        .add("member_reports", c.array(c.choice({member_checked.schema, member_error.schema})))
        .fields({{"package_id", "string"}, {"content_root", "string"}, {"fighter_revision", "string"},
                 {"package_bytes", "integer"}, {"cached", "boolean"}, {"saved", "boolean"},
                 {"installed", "boolean"}}, false);
    c.add({"project.build"}, build);
    c.add({"project.install"}, saved_draft(c)
        .fields({{"package_id", "string"}, {"installed_package_id", "string"}, {"accepted", "boolean"},
                 {"saved", "boolean"}, {"installed", "boolean"}})
        .add("status", c.enumeration({"installed"}))
        .add("replaced_package_id", c.nullable("string")));
    c.add({"packages.export"}, c.object({{"output", "string"}, {"package_id", "string"}, {"bytes", "integer"}}));
    c.add({"parameters.read"}, saved_draft(c).merge(source_status(c))
        .fields({{"text", "string"}, {"valid", "boolean"}})
        .fields({{"error_line", "integer"}}, false)
        .add("fields", c.array(c.object({{"name", "string"}, {"type", "string"}})
            .add("value", c.choice({c.type("number"), c.type("boolean")})).schema)));

    c.add({"presentation.edit"}, c.object({{"revision", "string"}, {"saved", "boolean"}}));
    auto portrait = [&] {
        return c.object({{"has_image", "boolean"}, {"max_png_bytes", "integer"}, {"max_image_dimension", "integer"}})
            .fields({{"bytes", "integer"}, {"width", "integer"}, {"height", "integer"},
                     {"sha256", "string"}, {"resource", "string"}, {"file", "string"}}, false)
            .add("framing", c.object({{"animation", "string"}, {"frame", "number"}, {"yaw", "number"},
                     {"pitch", "number"}, {"zoom", "number"}, {"pan_x", "number"}, {"pan_y", "number"}}).schema, false);
    };
    c.add({"presentation.read"}, c.object({{"project", "string"}, {"title", "string"}, {"revision", "string"},
                                           {"tile_name", "string"}, {"displayed_tile_name", "string"}})
        .add("card", portrait().schema)
        .add("tile", portrait().schema));
}
}
