#include "rule_engine.h"

#include <utility>
#include <print>

namespace nano_edr {

void RuleEngine::Subscribe(AlertSink sink) {
    sinks_.push_back(sink);
}

Detection RuleEngine::MakeDetection(const IRule& rule, const Event& event) const {
    std::string chain;
    if (model_ == nullptr)
    {
        chain = "";
    }
    else
    {
        chain = model_->DescribeChain(event.pid());
    }
    return Detection{rule.id(),
        rule.severity(),
        event.ts(),
        event.pid(),
        std::move(chain),
        rule.actions()
    };
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
            for (const auto& sink : sinks_) {
                sink(MakeDetection(*rule, event));
            }

            std::print("[DETECT] {}  {}  ts={} pid={}\n",
                    SeverityName(rule->severity()),
                    rule->id(),
                    event.raw_ts(),
                    event.pid());
        
        }
    }

    return detects;
}

}  // namespace nano_edr
