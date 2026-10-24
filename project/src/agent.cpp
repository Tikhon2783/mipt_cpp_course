#include "agent.h"

#include <print>
#include <vector>

#include "agent_rules.h"
#include "rules.h"

namespace nano_edr {

constexpr std::size_t kContextSize = 2;

Agent::Agent(std::size_t window_size, bool quiet) : events_window_(window_size), quiet_(quiet) {
    for (auto& rule : BuildAgentRules()) {
        engine_.AddRule(std::move(rule));
    }
    context_ptr_ = events_window_.head();
}

void Agent::HandleEvent(const Event& event) {
    std::vector<const IRule*> fired;
    engine_.ProcessEvent(event, &fired);

    bool detected = !fired.empty();
    for (const IRule* rule : fired) {
        std::print("[DETECT] {}  {}  ts={} pid={}\n",
                   SeverityName(rule->severity()),
                   rule->id(),
                   event.raw_ts(),
                   event.pid());
    }

    if (detected && !quiet_) {
        auto curr_context_size = std::min(kContextSize, events_window_.size());
        auto context_head = context_ptr_;
        for (size_t j = 0; j < curr_context_size; j++) {
            if (context_head == nullptr) {
                break;
            }
            std::print(
                "[CTX] -{}: ts={} type={} pid={}\n",
                curr_context_size - j, context_head->event.raw_ts(), context_head->event.type(), context_head->event.pid());
            context_head = context_head->next;
        }
    }

    events_window_.PushBack(event);
    if (context_ptr_ != nullptr && events_window_.size() > kContextSize) {
        context_ptr_ = context_ptr_->next;
    }
    ++total_;
    ++cnt[event.type()];
}

void Agent::OnSkipped() {}

void Agent::OnBroken() {}

void Agent::OnTick() {}

void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("всего событий {}, из них:\n", total_);
    for (auto t : cnt) {
        std::print("\t{}: {}\n", t.first, t.second);
    }
    std::print("правила, детектов:\n");
    for (std::size_t i = 0; i < engine_.size(); i++) {
        const IRule& rule = engine_.rule(i);
        std::print("  {}:\t{}\n", rule.id(), rule.hits());
    }
}

}  // namespace nano_edr
