#include "agent.h"

#include <print>

#include "agent_rules.h"
#include "rules.h"

namespace nano_edr {

constexpr std::size_t kContextSize = 2;

Agent::Agent(std::size_t window_size, bool quiet) : events_window_(window_size), quiet_(quiet) {
    rules_ = BuildAgentRules();
    context_ptr_ = events_window_.head();
}

void Agent::HandleEvent(const Event& event) {
    bool detected = false;
    for (size_t i = 0; i < rules_.size(); i++) {
        if (rules_[i].Check(event)) {
            detected = true;
            std::print("[DETECT] {}  {}  ts={} pid={}\n",
                SeverityName(rules_[i].severity()),
                rules_[i].id(),
                event.raw_ts(),
                event.pid());
        }
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

void Agent::PrintSummary() const {
    if (quiet_) {
        return;
    }
    std::print("всего событий {}, из них:\n", total_);
    for (auto t : cnt) {
        std::print("\t{}: {}\n", t.first, t.second);
    }
    std::print("правила, детектов:\n");
    for (const auto& rule : rules_) {
        std::print("  {}:\t{}\n", rule.id(), rule.hits());
    }
}

void Agent::Trampoline(const os_event* ev, void* ctx) noexcept {
    try {
        EventParts parts;
        parts.ts = std::to_string(ev->ts);
        if (ev->type != nullptr) {
            parts.type = ev->type;
        }
        if (ev->pid != 0) {
            parts.pid = std::to_string(ev->pid);
        }
        for (std::size_t i = 0; i < ev->field_count; i++) {
            parts.fields.push_back(Field{ev->fields[i].key, ev->fields[i].value});
        }

        Event event(parts);
        static_cast<Agent*>(ctx)->HandleEvent(event);
    } catch (const std::exception& e) {
        std::print(stderr, "пропущено событие: {}\n", e.what());
    } catch (...) {
        std::print(stderr, "пропущено событие: неизвестная ошибка\n");
    }
}

}  // namespace nano_edr
