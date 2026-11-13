#include <melee/ui/native_news_text.h>
#include <cstdio>
#include <cstring>
#include <string>

namespace {
NativeNewsText parsed;
unsigned checks;
#define CHECK(condition) do { ++checks; if (!(condition)) { std::fprintf(stderr, "line %u: %s\n", __LINE__, #condition); return 1; } } while (0)
std::string url(const char* source, unsigned index)
{ const auto& link = parsed.links[index]; return {source + link.url_start, link.url_length}; }
}
int main()
{
    const char* source = "# Help\n\nRead [the guide](https://example.com/docs_(test)?a=1&b=2).\n- Visit HTTP://example.com/x.";
    nativeNewsText_Parse(source, parsed);
    CHECK(!std::strcmp(parsed.body, "# Help\n\nRead the guide.\n- Visit HTTP://example.com/x."));
    CHECK(parsed.link_count == 2);
    CHECK(url(source, 0) == "https://example.com/docs_(test)?a=1&b=2");
    CHECK(url(source, 1) == "HTTP://example.com/x");
    CHECK(std::string(parsed.body + parsed.links[0].start, parsed.links[0].length) == "the guide");
    for (const char* unsafe : {"javascript:alert(1)", "file:///C:/Windows", "https://user:pass@example.com/",
                              "https://", "https://?q=test", "https://:80/", "https://example.com/\r\n", "https://example.com\\path"})
        CHECK(!nativeNewsText_ValidUrl(unsafe, std::strlen(unsafe)));
    source = "<img src=x onerror=alert(1)> [bad](javascript:alert(1)) [local](file:///x)";
    nativeNewsText_Parse(source, parsed);
    CHECK(parsed.link_count == 0 && !std::strcmp(parsed.body, source));
    source = "See (https://example.com/a_(b)). [日本語](https://example.com/日本語)";
    nativeNewsText_Parse(source, parsed);
    CHECK(parsed.link_count == 2);
    CHECK(url(source, 0) == "https://example.com/a_(b)");
    CHECK(!std::strcmp(parsed.body, "See (https://example.com/a_(b)). 日本語"));
    const std::string long_url = "https://example.com/" + std::string(2048, 'a');
    CHECK(!nativeNewsText_ValidUrl(long_url.data(), long_url.size()));
    std::string long_article(128010, 'a');
    nativeNewsText_Parse(long_article.c_str(), parsed);
    CHECK(std::strlen(parsed.body) == 128000 && parsed.link_count == 0);
    nativeNewsText_Parse(nullptr, parsed);
    CHECK(!*parsed.body && !parsed.link_count);
    std::printf("{\"status\":\"PASS\",\"checks\":%u}\n", checks); return 0;
}
