#include "rule_engine.h"

#include <memory>
#include <utility>

#include "copy_stats.h"

namespace nano_edr {

void RuleEngine::Subscribe(AlertSink sink) {
    sinks_.push_back(sink);
}

Detection RuleEngine::MakeDetection(const IRule& rule, const Event& event) const {
    std::string chain;
    if (model_ == nullptr) {
        chain = "";
    } else {
        chain = model_->DescribeChain(event.pid());
    }
    Detection detection;
    detection.rule = rule.id();
    detection.severity = rule.severity();
    detection.ts = event.ts();
    detection.pid = event.pid();
    detection.chain = std::move(chain);
    detection.actions = rule.actions();
    detection.evidence.push_back(std::make_unique<Event>(event));
    ++copy_stats().evidence_copied;
    return detection;
}

void RuleEngine::AddRule(std::unique_ptr<IRule> rule) {
    rules_.push_back(std::move(rule));
}

std::size_t RuleEngine::ProcessEvent(const Event& event,
                                     std::vector<const IRule*>* fired) {
    std::size_t detects = 0;
    for (const auto& rule : rules_) {
        if (rule->Check(event)) {
            ++detects;
            if (fired != nullptr) {
                fired->push_back(rule.get());
            }
            const Detection detection = MakeDetection(*rule, event);
            for (const auto& sink : sinks_) {
                sink(detection);
            }
        }
    }

    return detects;
}

}  // namespace nano_edr
