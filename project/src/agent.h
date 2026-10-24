#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "event.h"
#include "event_list.h"
#include "os.h"
#include "rule.h"

namespace nano_edr {

class Agent {
 public:
    Agent(std::size_t window_size, bool quiet);  // из --window-size и --quiet

    void HandleEvent(const Event& event);
    void PrintSummary() const;  // сводка в конце прогона

    static void Trampoline(const os_event* ev, void* ctx) noexcept;

 private:
    EventList events_window_;
    std::vector<MatchRule> rules_;  // в конструкторе: BuildAgentRules()
    const nano_edr::EventNode* context_ptr_;
    bool quiet_;
    int total_ = 0;
    std::map<std::string, int> cnt;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H
