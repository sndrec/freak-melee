#include <melee/gm/native_feedback_json.h>
#include "logging.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>

namespace aurora { AuroraConfig g_config{}; }

namespace {
std::string local_message, report_message, report_module;

void require(bool condition, const char* message)
{
    if (!condition) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}

void test_warning()
{
    aurora::g_config.logCallback = [](AuroraLogLevel, const char*, const char* message, unsigned length) {
        local_message.assign(message, length);
    };
    aurora::g_config.diagnosticWarningCallback = [](AuroraLogLevel, const char* module, const char* text, unsigned length) {
        report_module = module;
        report_message.assign(text, length);
    };
    constexpr aurora::Module log{"aurora::input"};
    log.info("Loading {}", "Local Path");
    require(report_module.empty(), "Informational messages entered the feedback ring");
    log.warn("Could not open {} for controller {}", "Local Path", 2);
    require(local_message == "Could not open Local Path for controller 2", "Local warning lost its details");
    require(report_module == "aurora::input" && report_message == "Could not open {} for controller {}",
            "Feedback should receive only the authored warning template");
}

void test_document_budget()
{
    NativeFeedbackJsonBudget budget{64 * 1024, false};
    auto allocator = nativeFeedbackJson_Allocator(&budget);
    auto* doc = yyjson_mut_doc_new(&allocator);
    auto* root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);
    require(yyjson_mut_obj_add_strcpy(doc, root, "scene", "practice"), "Could not build normal feedback");
    size_t length = 0;
    char* bytes = yyjson_mut_write_opts(doc, 0, &allocator, &length, nullptr);
    require(bytes && std::string(bytes, length) == R"({"scene":"practice"})" && !budget.exhausted,
            "Could not serialize normal feedback within budget");
    allocator.free(allocator.ctx, bytes);
    std::string large_field(65536, 'e');
    require(!yyjson_mut_obj_add_strcpy(doc, root, "name", large_field.c_str()) && budget.exhausted,
            "An oversized copied field should invalidate the collection");
    require(!allocator.malloc(allocator.ctx, 1), "An exhausted collection resumed allocation");
    yyjson_mut_doc_free(doc);

    budget = {64 * 1024, false};
    doc = yyjson_mut_doc_new(&allocator);
    root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);
    require(yyjson_mut_obj_add_str(doc, root, "name", large_field.c_str()), "Could not borrow test string");
    require(!yyjson_mut_write_opts(doc, 0, &allocator, &length, nullptr) && budget.exhausted,
            "Serialization must use the same budget as collection");
    yyjson_mut_doc_free(doc);
}
}

int main()
{
    test_warning();
    test_document_budget();
    puts("Feedback warning privacy and collection budgets passed");
}
