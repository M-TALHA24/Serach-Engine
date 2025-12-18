#include "text_normalizer.h"
#include <regex>
#include <algorithm>
#include <cctype>


std::string TextNormalizer::normalize(const std::string &text) {
    if (text.empty()) return "";

    std::string s = text;

    // 2. Lowercase
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    // 3. Separate @ and #
    s = std::regex_replace(s, std::regex("([@#])"), " $1 ");

    // 4. Remove apostrophes
    s.erase(std::remove(s.begin(), s.end(), '\''), s.end());

    // 5. Tokenize math operators and parentheses
    s = std::regex_replace(s, std::regex("([+\\-*/=<>])"), " $1 ");
    s = std::regex_replace(s, std::regex("([()])"), " $1 ");

    // 6. Tokenize dollar sign
    s = std::regex_replace(s, std::regex("\\$"), " $ ");

    // 7. Replace decimal numbers (12.34 → 12 34)
    s = std::regex_replace(s, std::regex("(\\d+)\\.(\\d+)"), "$1 $2");

    // Remove commas inside numbers
    s = std::regex_replace(s, std::regex(","), "");

    // 8. Remove all other punctuation
    // Allow: a-z 0-9 space + - * / = < > ( ) $ @ # ä ö ü ß
    s = std::regex_replace(
        s,
        std::regex("[^a-z0-9\\s+\\-*/=<>()\\$@#äöüß]"),
        " "
    );

    // 9. Split hyphenated words
    s = std::regex_replace(s, std::regex("\\s*-\\s*"), " ");

    // 10. Collapse multiple spaces
    s = std::regex_replace(s, std::regex("\\s+"), " ");

    // Trim leading/trailing space
    if (!s.empty() && s.front() == ' ') s.erase(0, 1);
    if (!s.empty() && s.back() == ' ') s.pop_back();

    return s;
}
