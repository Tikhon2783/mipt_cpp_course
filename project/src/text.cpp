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
    if (s1.size() != s2.size())
    {
        return false;
    }
    for (size_t i = 0; i < s1.size(); i++)
    {
        if (c_to_lower(s1[i]) != c_to_lower(s2[i]))
        {
            return false;
        }
    }
    return true;
}

// todo fix unnecesary copying
bool string_starts_with(const std::string& s, const std::string& pref) {
    if (s.size() < pref.size())
    {
        return false;
    }
    for (size_t i = 0; i < pref.size(); i++)
    {
        if (c_to_lower(s[i]) != c_to_lower(pref[i]))
        {
            return false;
        }
    }
    return true;
}
bool string_contains(const std::string& s, const std::string& substr) {
    return to_lower(s).find(to_lower(substr)) != std::string::npos;
}

bool string_ends_with(const std::string& s, const std::string& suff) {
    if (s.size() < suff.size())
    {
        return false;
    }
    for (size_t i = 0; i < suff.size(); i++)
    {
        if (c_to_lower(s[s.size() - suff.size() + i]) != c_to_lower(suff[i]))
        {
            return false;
        }
    }
    return true;
}

}  // namespace nano_edr
