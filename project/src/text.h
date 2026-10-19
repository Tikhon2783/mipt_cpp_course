#include <string>

#ifndef NANO_EDR_KIT_TEXT_H
#define NANO_EDR_KIT_TEXT_H

namespace nano_edr {

char c_to_lower(unsigned char c);

std::string to_lower(const std::string& s);

bool string_equals(const std::string& s1, const std::string& s2);

bool string_contains(const std::string& s, const std::string& substr);

bool string_ends_with(const std::string& s, const std::string& suff);

}  // namespace nano_edr

#endif  // NANO_EDR_KIT_TEXT_H
