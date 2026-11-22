#include "rule.h"

#include <utility>

namespace nano_edr {

RuleBase::RuleBase(const std::string& id, Severity severity) : id_(id), severity_(severity) {}

void RuleBase::AddAction(const std::string& action) {
    actions_.push_back(action);
}

MatchRule::MatchRule(const std::string& id, Severity severity) : RuleBase(id, severity) {}

void MatchRule::AddCondition(std::unique_ptr<ICondition> condition) {
    conditions_.push_back(std::move(condition));
}

bool MatchRule::Check(const Event& event) {
    for (const auto& condition : conditions_) {
        if (!condition->Matches(event)) {
            return false;
        }
    }

    CountHit();
    return true;
}

SequenceRule::SequenceRule(const std::string& id, Severity severity, uint64_t window_ms)
    : RuleBase(id, severity), window_ms_(window_ms) {}

void SequenceRule::SetFirst(std::unique_ptr<ICondition> condition) {
    first_ = std::move(condition);
}

void SequenceRule::SetSecond(std::unique_ptr<ICondition> condition) {
    second_ = std::move(condition);
}

bool SequenceRule::Linked(const Pending& first, const Event& second) const {
    if (child_of_prev_) {
        if (model_ == nullptr) {
            return false;
        }
        const ProcessRecord* process = model_->FindProcess(second.pid());
        return process != nullptr && process->ppid == first.pid;
    }
    if (same_pid_) {
        return first.pid == second.pid();
    }
    return true;
}

bool SequenceRule::Check(const Event& event) {
    uint64_t now = event.ts().ms;

    for (auto it = pending_.begin(); it != pending_.end();) {
        if (now - it->ts_ms > window_ms_) {
            it = pending_.erase(it);
        } else {
            break;
        }
    }

    if (second_ && second_->Matches(event)) {
        for (auto it = pending_.begin(); it != pending_.end(); ++it) {
            if (!Linked(*it, event)) {
                continue;
            }
            pending_.erase(it);
            CountHit();
            return true;
        }
    }

    if (first_ && first_->Matches(event) && second_) {
        pending_.push_back(Pending{now, event.pid()});
    }
    return false;
}

// src/rule.cpp, шаг 7
LambdaRule::LambdaRule(const std::string& id, Severity severity, Predicate)
    : RuleBase(id, severity) {}
bool LambdaRule::Check(const Event&) { return false; }

}  // namespace nano_edr
