#include "field_traits.h"

#include <charconv>

#include "text.h"

namespace nano_edr {

bool FieldTraits<std::string>::Parse(const std::string& text, std::string* out) {
    *out = text;
    return true;
}

bool FieldTraits<uint64_t>::Parse(const std::string& text, uint64_t* out) {
    uint64_t val = 0;
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), val);
    if (ec != std::errc() || ptr != text.data() + text.size()) {
        return false;
    }
    *out = val;
    return true;
}

bool FieldTraits<bool>::Parse(const std::string& text, bool* out) {
    if (string_equals(text, "1") || string_equals(text, "true") || string_equals(text, "yes")) {
        *out = true;
        return true;
    }
    if (string_equals(text, "0") || string_equals(text, "false") || string_equals(text, "no")) {
        *out = false;
        return true;
    }
    return false;
}

}  // namespace nano_edr
