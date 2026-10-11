#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <map>

#include "event.h"
#include "event_list.h"
#include "os.h"

namespace nano_edr {

class Agent {
 public:
    Agent(std::size_t window_size, bool quiet);  // из --window-size и --quiet

    void HandleEvent(const Event& event);
    void PrintSummary() const;  // сводка в конце прогона

    static void Trampoline(const os_event* ev, void* ctx) noexcept;

 private:
    EventList events_window_;  // последние события, не больше window_size
    bool quiet_;
    int total_ = 0;
    std::map<std::string, int> cnt;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H
