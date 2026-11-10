#include "agent.h"

#include <algorithm>
#include <print>
#include <vector>

#include "agent_rules.h"
#include "rules.h"

namespace nano_edr {

constexpr std::size_t kContextSize = 2;

Agent::Agent(bool quiet) : engine_(BuildAgentRules()), quiet_(quiet) {}

void Agent::HandleEvent(const Event& event) {
    constexpr std::string_view fmt = "[CTX] -{}: {} {} pid={}\n";

    std::vector<const IRule*> fired;
    size_t detects = engine_.ProcessEvent(event, &fired);

    if (detects > 0 && !quiet_) {
        for (size_t i = std::min(kContextSize, window_.size()); i > 0; i--) {
            const WindowEntry& entry = window_[window_.size() - i];
            std::print(
                fmt,
                i,
                entry.ts.ms,
                entry.type,
                entry.pid);
        }
    }

    for (const IRule* rule : fired) {
        std::print("[DETECT] {}  {}  ts={} pid={}\n",
                   SeverityName(rule->severity()),
                   rule->id(),
                   event.raw_ts(),
                   event.pid());
    }

    window_.PushBack(WindowEntry{event.ts(), event.type(), event.pid()});
    ++total_;
    ++cnt[event.type()];
}

void Agent::OnSkipped() {
    ++skipped_;
}

void Agent::OnBroken() {
    ++broken_;
}

void Agent::OnTick() {}

void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("[STAT] СВОДКА:\n");
    std::print("[STAT] всего событий {}, из них:\n", total_);
    for (auto t : cnt) {
        std::print("\t{}: {}\n", t.first, t.second);
    }
    std::print("[STAT] пропущено строк {}, битых {}\n", skipped_, broken_);
    std::print("[STAT] в окне {} из {} мест\n", window_.size(), window_.capacity());
    std::print("[STAT] правила, детектов:\n");
    for (std::size_t i = 0; i < engine_.size(); i++) {
        const IRule& rule = engine_.rule(i);
        std::print("[STAT]   {}:\t{}\n", rule.id(), rule.hits());
    }
}

}  // namespace nano_edr
