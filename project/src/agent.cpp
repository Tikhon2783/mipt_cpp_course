#include "agent.h"

#include <algorithm>
#include <print>
#include <vector>

#include "agent_rules.h"
#include "rules.h"

namespace nano_edr {

constexpr std::size_t kContextSize = 2;

Agent::Agent(bool quiet) : engine_(BuildAgentRules(&model_)), quiet_(quiet) {
    engine_.SetModel(&model_);
}

void Agent::HandleEvent(const Event& event) {
    constexpr std::string_view fmt = "[CTX] -{}: {} {} pid={}\n";

    model_.Observe(event);

    ctx_printed_ = false;
    std::vector<const IRule*> fired;
    engine_.ProcessEvent(event, &fired);

    window_.PushBack(WindowEntry{event.ts(), event.type(), event.pid()});
    ++total_;
    ++cnt[event.type()];
}

void Agent::Subscribe(AlertSink sink) {
    FanOutSink all;
    all.Add([this](const Detection&) { PrintContextOnce(); });
    all.Add(std::move(sink));
    engine_.Subscribe(std::move(all));
}

void Agent::PrintContextOnce() {
    constexpr std::string_view fmt = "[CTX] -{}: {} {} pid={}\n";

    if (quiet_ || ctx_printed_)
    {
        return;
    }

    ctx_printed_ = true;
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
    std::print("[STAT] в окне контекста {} из {} мест\n", window_.size(), window_.capacity());
    std::print("[STAT] в модели процессов {}, файлов {}\n", model_.process_count(), model_.file_count());
    std::print("[STAT] правила, детектов:\n");
    for (std::size_t i = 0; i < engine_.size(); i++) {
        const IRule& rule = engine_.rule(i);
        std::print("[STAT]   {}:\t{}\n", rule.id(), rule.hits());
    }
}

}  // namespace nano_edr
