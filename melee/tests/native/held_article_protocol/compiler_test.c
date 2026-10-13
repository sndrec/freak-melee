#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(value) do { ++checks; assert(value); } while (0)
static bool generate(const char* source, FILE* output, NativeFighterScriptError* error)
{
    return nativeFighterScript_GenerateC(source, strlen(source),
        "held_article_protocol", NULL, NULL, NULL, output, error);
}
static char* read_file(FILE* file)
{
    CHECK(!fseek(file, 0, SEEK_END)); long size = ftell(file); CHECK(size >= 0); rewind(file);
    char* text = malloc((size_t)size + 1); CHECK(text);
    CHECK(fread(text, 1, (size_t)size, file) == (size_t)size); text[size] = 0; return text;
}
static void reject_body(const char* status, const char* request, const char* diagnostic)
{
    char source[4096];
    snprintf(source, sizeof(source), "article It_Kind_Peach_Parasol states 1:\n"
        "    state int observed\n    state 0 anim -1:\n"
        "    on held_status:\n%s    on held_request:\n%s", status, request);
    FILE* output = tmpfile(); CHECK(output);
    NativeFighterScriptError error = {0};
    if (generate(source, output, &error)) { fprintf(stderr, "Unexpected acceptance:\n%s", source); abort(); }
    ++checks; CHECK(error.line != 0);
    if (diagnostic) CHECK(strstr(error.message, diagnostic));
    fclose(output);
}
static void negative(void)
{
    const char* status[] = {
        "            return true\n", "            return 1.0\n",
        "            return event.action\n", "            return event.frame_rate\n",
        "            item.state.observed = 1\n            return 6\n",
        "            item.animation_rate = 1\n            return 6\n",
        "            item.remove()\n            return 6\n",
        "            return random.int(7)\n",
        "            let int value = random.int(7)\n            return value\n",
        "            if random.int(7) > 0:\n                return 4\n            return 6\n",
        "            return int(targets.call_bool(item.current_owner, \"mutate\"))\n",
        "            let bool found = targets.call_bool(item.current_owner, \"mutate\")\n            return 6\n",
        "            mutate()\n            return 6\n",
        "            while true:\n                pass\n            return 6\n",
        "            return int(holder.held_frame())\n",
    };
    for (unsigned i = 0; i < sizeof(status)/sizeof(*status); ++i)
        reject_body(status[i], "            return false\n", "held_status is a read-only query");
    const char* request[] = {
        "            return 1\n", "            return 0.0\n",
        "            let int bad = event.duration\n            return false\n",
        "            let int bad = event.frame_rate\n            return false\n",
        "            return event.unknown\n", "            return event.action() == 4\n",
        "            let int bad = targets.held_id(1.5)\n            return false\n",
        "            let int bad = targets.held_motion(true)\n            return false\n",
        "            let int bad = targets.held_id()\n            return false\n",
        "            let int bad = targets.held_id(1, 2)\n            return false\n",
        "            let int bad = holder.held_motion.extra\n            return false\n",
        "            holder.held_motion = 2\n            return false\n",
    };
    for (unsigned i = 0; i < sizeof(request)/sizeof(*request); ++i)
        reject_body("            return -1\n", request[i], NULL);
    const char* contexts[] = {
        "article It_Kind_Peach_Parasol states 1:\n    state 0 anim -1:\n"
            "        on physics:\n            item.animation_rate = event.duration\n",
        "article It_Kind_Peach_Parasol states 1:\n    state 0 anim -1:\n"
            "    on signal request:\n            let int bad = event.action\n",
        "article It_Kind_Peach_Parasol states 1:\n    func bad():\n"
            "        let float bad = event.frame_rate\n    state 0 anim -1:\n",
        "item Ordinary damage 1 angle 1 lifetime 1:\n    on tick:\n        item.life = holder.held_frame\n",
        "article It_Kind_Peach_Parasol states 1:\n    state 0 anim -1:\n"
            "    on held_status:\n            return 4\n",
        "article It_Kind_Peach_Parasol states 1:\n    state 0 anim -1:\n"
            "    on held_request:\n            return false\n",
        "article It_Kind_Peach_Parasol states 1:\n    state 0 anim -1:\n"
            "    on held_status:\n            return 4\n    on held_status:\n            return 6\n"
            "    on held_request:\n            return false\n",
    };
    for (unsigned i = 0; i < sizeof(contexts)/sizeof(*contexts); ++i) {
        FILE* output = tmpfile(); CHECK(output); NativeFighterScriptError error = {0};
        CHECK(!generate(contexts[i], output, &error)); fclose(output);
    }
}
int main(int argc, char** argv)
{
    if (argc != 2 && argc != 3) return 2;
    FILE* input = fopen(argv[1], "rb"); CHECK(input); char* source = read_file(input); fclose(input);
    FILE* output = argc == 3 ? fopen(argv[2], "w+b") : tmpfile(); CHECK(output);
    NativeFighterScriptError error = {0};
    if (!generate(source, output, &error)) { fprintf(stderr, "line %u: %s\n", error.line, error.message); return 1; }
    ++checks; char* code = read_file(output);
    CHECK(strstr(code, "int32_t held_action, float held_frame_rate, float held_duration"));
    CHECK(strstr(code, "static int32_t scripted_article_0_logic_"));
    CHECK(strstr(code, "return -1;"));
    CHECK(strstr(code, "host->target_held_id == 0"));
    CHECK(strstr(code, "host->target_held_kind == 0"));
    CHECK(strstr(code, "host->target_held_motion == 0"));
    CHECK(strstr(code, "host->target_held_frame == 0"));
    CHECK(strstr(code, "host->item_current_owner == 0"));
    CHECK(strstr(code, "host->callback_source_alive == 0"));
    CHECK(!strstr(code, "ftGetParasolStatus") && !strstr(code, "it_802BDD40"));
    free(code); free(source); fclose(output); negative();
    printf("Held article protocol compiler PASS %u checks\n", checks); return 0;
}
