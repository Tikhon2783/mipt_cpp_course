#include <string>
#include <vector>

#include "conditions.h"
#include "event.h"
#include "fields.h"
#include "text.h"

namespace nano_edr {

bool FieldEquals::operator()(const Event& event) const {
    auto val = FindField(event, key_);
    if (val == nullptr)
    {
        return false;
    }
    return string_equals(*val, value_);
}

bool FieldContains::operator()(const Event& event) const {
    auto val = FindField(event, key_);
    if (val == nullptr)
    {
        return false;
    }
    return string_contains(*val, needle_);
}

bool FieldEndsWith::operator()(const Event& event) const {
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
    return false;
}

}  // namespace nano_edr
