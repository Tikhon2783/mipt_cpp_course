#include <string>
#include <vector>

#include "conditions.h"
#include "event.h"
#include "fields.h"
#include "text.h"

namespace nano_edr {

bool CheckScriptHostFromTemp(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }

    const std::string* image = FindField(event, "image");
    if (!image) {
        throw std::invalid_argument("image is required");
    }

    std::string image_normalized = NormalizePath(*image);

    if (image_normalized.find("wscript.exe") == std::string::npos && image_normalized.find("cscript.exe") == std::string::npos) {
        return false;
    }

    return CommandLineContains(event, "\\appdata\\local\\temp\\") || CommandLineContains(event, "\\windows\\temp\\");
}

bool FieldEquals::operator()(const Event& event) const {
    auto val = FindField(event, key_);
    if (val == nullptr)
    {
        return false;
    }
    return string_contains(*val, value_);
}

bool FieldContains::operator()(const Event& event) const {
    auto val = FindField(event, key_);
    if (val == nullptr)
    {
        return false;
    }
    return string_contains(*val, needle_);
}

bool FieldEndsWith::operator()(const Event& event) const{
    auto val = FindField(event, key_);
    if (val == nullptr)
    {
        return false;
    }
    return string_ends_with(*val, suffix_);
}

bool FieldInList::operator()(const Event& event) const {
    for (const auto& val_ : values_)
    {
        if (FieldEquals(key_, val_)(event))
        {
            return true;
        }
    }
    return false;
}


Condition Condition::Equals(const std::string& key,
                        const std::vector<std::string>& values) {
                            return Condition(Match::kEquals, key, values);
                        }
Condition Condition::Contains(const std::string& key,
                            const std::vector<std::string>& values) {
                            return Condition(Match::kContains, key, values);
                        }
Condition Condition::EndsWith(const std::string& key,
                            const std::vector<std::string>& values) {
                            return Condition(Match::kEndsWith, key, values);
                        }

bool Condition::operator()(const Event& event) const {
    switch (match_)
    {
    case Match::kEquals:
        return FieldInList(key_, values_)(event);
    case Match::kContains:
        for (const auto& val_ : values_)
        {
            if (FieldContains(key_, val_)(event))
            {
                return true;
            }
        }
        return false;
    case Match::kEndsWith:
        for (const auto& val_ : values_)
            {
                if (FieldEndsWith(key_, val_)(event))
                {
                    return true;
                }
            }
            return false;
    }
}

}  // namespace nano_edr
