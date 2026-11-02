#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <map>
#include <string>

#include "event.h"
#include "event_list.h"
#include "event_source.h"
#include "rule_engine.h"

namespace nano_edr {

class Agent : public IEventHandler {
 public:
    Agent(std::size_t window_size, bool quiet);  // из --window-size и --quiet

    void HandleEvent(const Event& event) override;
    void OnSkipped() override;
    void OnBroken() override;
    void OnTick() override;

    void PrintSummary() const;  // сводка в конце прогона

 private:
    EventList events_window_;
    RuleEngine engine_;  // в конструкторе: BuildAgentRules()
    const nano_edr::EventNode* context_ptr_;
    bool quiet_;
    int total_ = 0;
    int skipped_ = 0;
    int broken_ = 0;
    std::map<std::string, int> cnt;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H
