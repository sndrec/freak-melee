#include "native_news_text.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iterator>

namespace {
unsigned scheme(const char* text, size_t length)
{
    for (const char* value : {"https://", "http://"}) {
        const auto count = std::strlen(value);
        if (length < count) continue;
        bool matches = true;
        for (size_t i = 0; i < count; ++i) if (std::tolower(static_cast<unsigned char>(text[i])) != value[i]) matches = false;
        if (matches) return unsigned(count);
    }
    return 0;
}
bool delimiter(unsigned char c) { return c <= 32 || c == '<' || c == '>' || c == '"' || c == '\''; }
unsigned literal_end(const char* source, unsigned start, unsigned size)
{
    unsigned end = start;
    int parentheses = 0, brackets = 0;
    while (end < size && !delimiter(static_cast<unsigned char>(source[end]))) {
        const char c = source[end];
        if (c == '(') ++parentheses;
        if (c == ')' && --parentheses < 0) break;
        if (c == '[') ++brackets;
        if (c == ']' && --brackets < 0) break;
        ++end;
    }
    while (end > start && std::strchr(".,;!", source[end-1])) --end;
    return end;
}
}

bool nativeNewsText_ValidUrl(const char* text, size_t length)
{
    if (!text || length > 2048) return false;
    const unsigned prefix = scheme(text, length);
    if (!prefix || length <= prefix) return false;
    size_t authority_end = prefix;
    while (authority_end < length && !std::strchr("/?#", text[authority_end])) ++authority_end;
    if (authority_end == prefix) return false;
    if (text[prefix] == ':') return false;
    for (size_t i = prefix; i < length; ++i) {
        if (delimiter(static_cast<unsigned char>(text[i])) || text[i] == '\\' || !text[i]) return false;
        if (i < authority_end && text[i] == '@') return false;
    }
    return true;
}

void nativeNewsText_Parse(const char* source, NativeNewsText& result)
{
    result.link_count = 0; result.body[0] = 0;
    if (!source) return;
    const unsigned size = unsigned(std::min(std::strlen(source), sizeof(result.body)-1));
    unsigned input = 0, output = 0;
    while (input < size) {
        unsigned label = input, label_end = input, url = input, url_end = input, next = input;
        if (source[input] == '[') {
            label = input + 1; label_end = label;
            while (label_end < size && source[label_end] != ']' && source[label_end] != '\n') ++label_end;
            if (label_end > label && label_end + 2 < size && source[label_end] == ']' && source[label_end+1] == '(') {
                url = label_end + 2; url_end = url;
                int depth = 1;
                while (url_end < size && !delimiter(static_cast<unsigned char>(source[url_end]))) {
                    if (source[url_end] == '(') ++depth;
                    if (source[url_end] == ')' && !--depth) break;
                    ++url_end;
                }
                if (!depth && url_end < size && nativeNewsText_ValidUrl(source + url, url_end - url)) next = url_end + 1;
            }
        }
        if (next == input && scheme(source + input, size - input) &&
            (!input || !std::isalnum(static_cast<unsigned char>(source[input-1])))) {
            label = url = input; label_end = url_end = literal_end(source, input, size);
            if (nativeNewsText_ValidUrl(source + url, url_end - url)) next = url_end;
        }
        if (next > input && result.link_count < std::size(result.links)) {
            const unsigned length = label_end - label;
            result.links[result.link_count++] = {output, length, url, url_end-url};
            std::memcpy(result.body + output, source + label, length); output += length; input = next;
        } else result.body[output++] = source[input++];
    }
    result.body[output] = 0;
}
