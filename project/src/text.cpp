#include <format>

#include "text.h"

namespace nano_edr {

char c_to_lower(unsigned char c) {
    return std::tolower(c);
}

std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), c_to_lower);
    return result;
}

bool string_equals(const std::string& s1, const std::string& s2) {
    return s1 == s2;
}

bool string_contains(const std::string& s, const std::string& substr) {
    return s.contains(substr);
}

bool string_ends_with(const std::string& s, const std::string& suff) {
    return s.length() > suff.length() && s.substr(s.length() - suff.length()) == suff;
}

}  // namespace nano_edr
