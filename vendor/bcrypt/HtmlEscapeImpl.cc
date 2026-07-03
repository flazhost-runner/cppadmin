// Implementation of sanitizeRichHtml() declared in include/helpers/HtmlEscape.h
#include "include/helpers/HtmlEscape.h"
#include <regex>

std::string sanitizeRichHtml(const std::string &html) {
    if (html.empty()) return html;
    std::string out = html;

    // Remove <script> blocks entirely
    out = std::regex_replace(out, std::regex("<script[^>]*>[\\s\\S]*?</script>",
                                             std::regex::icase), "");
    // Remove event-handler attributes (onclick, onerror, onload, etc.)
    out = std::regex_replace(out, std::regex("\\s+on\\w+\\s*=\\s*\"[^\"]*\"",
                                             std::regex::icase), "");
    out = std::regex_replace(out, std::regex("\\s+on\\w+\\s*=\\s*'[^']*'",
                                             std::regex::icase), "");
    // Remove javascript: URLs
    out = std::regex_replace(out, std::regex("javascript\\s*:",
                                             std::regex::icase), "");
    return out;
}
