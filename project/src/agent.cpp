#include "agent.h"

#include <algorithm>
#include <memory>
#include <print>
#include <utility>
#include <vector>

#include "agent_rules.h"
#include "dedup_sink.h"
#include "rules.h"

namespace nano_edr {

constexpr std::size_t kContextSize = 2;

Agent::Agent(bool quiet, uint64_t dedup_period_ms)
    : engine_(BuildAgentRules(&model_)),
      quiet_(quiet),
      dedup_period_ms_(dedup_period_ms) {
    engine_.SetModel(&model_);
}

void Agent::HandleEvent(const Event& event) {
    constexpr std::string_view fmt = "[CTX] -{}: {} {} pid={}\n";

    model_.Observe(event);

    if (responder_ != nullptr && responder_->Recognize(event)) {
        WindowEntry own_entry;
        own_entry.ts = event.ts();
        own_entry.type = event.type();
        own_entry.pid = event.pid();
        window_.PushBack(std::move(own_entry));
        return;
    }

    ctx_printed_ = false;
    std::vector<const IRule*> fired;
    engine_.ProcessEvent(event, &fired);

    WindowEntry entry;
    entry.ts = event.ts();
    entry.type = event.type();
    entry.pid = event.pid();
    window_.PushBack(std::move(entry));
    ++total_;
    ++cnt[event.type()];
}

void Agent::Subscribe(AlertSink sink) {
    FanOutSink all;
    all.Add([this](const Detection&) { PrintContextOnce(); });
    all.Add(std::move(sink));
    engine_.Subscribe(DedupSink(std::move(all), dedup_period_ms_, &suppressed_));
}

void Agent::EnableResponse(OsHandle* os, bool dry_run,
                           const std::string& quarantine_dir) {
    planner_ = std::make_unique<ResponsePlanner>(&model_, quarantine_dir);
    responder_ = std::make_unique<Responder>(os, dry_run);
    engine_.Subscribe([this](const Detection& detection) {
        responder_->Execute(planner_->Plan(detection));
    });
}

void Agent::PrintContextOnce() {
    constexpr std::string_view fmt = "[CTX] -{}: {} {} pid={}\n";

    if (quiet_ || ctx_printed_) {
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
    if (suppressed_ > 0) {
        std::print("[STAT] подавлено дубликатов {} (срок {} мс)\n", suppressed_, dedup_period_ms_);
    }
    std::print("[STAT] правила, детектов:\n");
    for (std::size_t i = 0; i < engine_.size(); i++) {
        const IRule& rule = engine_.rule(i);
        std::print("[STAT]   {}:\t{}\n", rule.id(), rule.hits());
    }
    if (responder_ != nullptr) {
        const ResponseStats& stats = responder_->stats();
        std::print("[STAT] действий запланировано {}, повторов отброшено {}\n",
                   stats.planned, stats.skipped_repeat);
        std::print("[STAT] завершено процессов {}, изолировано файлов {}, удалено файлов {}\n",
                   stats.killed, stats.quarantined, stats.deleted);
        if (stats.own_events > 0) {
            std::print("[STAT] своих событий {}\n", stats.own_events);
        }
    }
}

}  // namespace nano_edr
