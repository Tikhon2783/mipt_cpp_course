#include "conditions.h"

#include <string>
#include <utility>
#include <vector>

#include "event.h"
#include "fields.h"
#include "text.h"

namespace nano_edr {

bool FieldEquals::Matches(const Event& event) const {
    auto val = FindField(event, key_);
    if (val == nullptr) {
        return false;
    }
    return string_equals(*val, value_);
}

bool FieldContains::Matches(const Event& event) const {
    auto val = FindField(event, key_);
    if (val == nullptr) {
        return false;
    }
    return string_contains(*val, needle_);
}

bool FieldEndsWith::Matches(const Event& event) const {
    auto val = FindField(event, key_);
    if (val == nullptr) {
        return false;
    }
    return string_ends_with(*val, suffix_);
}

bool FieldInList::Matches(const Event& event) const {
    auto val = FindField(event, key_);
    if (val == nullptr) {
        return false;
    }
    for (const auto& v : values_) {
        if (string_equals(*val, v)) {
            return true;
        }
    }
    return false;
}

bool EventTypeIs::Matches(const Event& event) const {
    for (EventType t : types_) {
        if (event.event_type() == t) {
            return true;
        }
    }
    return false;
}

void AllOf::Add(std::unique_ptr<ICondition> condition) {
    parts_.push_back(std::move(condition));
}

bool AllOf::Matches(const Event& event) const {
    for (const auto& part : parts_) {
        if (!part->Matches(event)) {
            return false;
        }
    }
    return true;
}

void AnyOf::Add(std::unique_ptr<ICondition> condition) {
    parts_.push_back(std::move(condition));
}

bool AnyOf::Matches(const Event& event) const {
    for (const auto& part : parts_) {
        if (part->Matches(event)) {
            return true;
        }
    }
    return false;
}

}  // namespace nano_edr
