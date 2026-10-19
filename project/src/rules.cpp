#include "rules.h"

namespace nano_edr {

    
const char* SeverityName(Severity severity) {
    switch (severity) {
        case Severity::kLow:
            return "low";
        case Severity::kMedium:
            return "medium";
        case Severity::kHigh:
            return "high";
        case Severity::kCritical:
            return "critical";
        default:
            return "?";
    }
}

size_t CheckRules(const Event& event, const Rule* rules, size_t rule_count) {
    size_t detects = 0;
    for (size_t i = 0; i < rule_count; i++) {
        if (rules[i].check(event)) {
            ++detects;
            printf("[DETECT] %s  %s  ts=%s pid=%s\n",
                   SeverityName(rules[i].severity),
                   rules[i].id,
                   event.raw_ts().c_str(),
                   event.pid().c_str());
        }
    }
    return detects;
}

}  // namespace nano_edr
