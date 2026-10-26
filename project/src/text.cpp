#include "text.h"

#include <format>

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
    return to_lower(s1) == to_lower(s2);
}

bool string_contains(const std::string& s, const std::string& substr) {
    return to_lower(s).find(to_lower(substr)) != std::string::npos;
}

bool string_ends_with(const std::string& s, const std::string& suff) {
    std::string lower_s = to_lower(s);
    std::string lower_suff = to_lower(suff);
    return lower_s.length() >= lower_suff.length() && lower_s.substr(lower_s.length() - lower_suff.length()) == lower_suff;
}

}  // namespace nano_edr
