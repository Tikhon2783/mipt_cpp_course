#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <map>
#include <string>

#include "event.h"
#include "event_source.h"
#include "ring_buffer.h"
#include "rule_engine.h"

namespace nano_edr {

struct WindowEntry {
    Timestamp ts;
    std::string type;
    std::string pid;
};

class Agent : public IEventHandler {
 public:
    static constexpr std::size_t kWindowCapacity = 256;

    explicit Agent(bool quiet);

    void HandleEvent(const Event& event) override;
    void OnSkipped() override;
    void OnBroken() override;
    void OnTick() override;

    void PrintSummary() const;

    void Agent::Subscribe(AlertSink sink);
    
private:
    void Agent::PrintContextOnce();

    RingBuffer<WindowEntry, kWindowCapacity> window_;
    EntityModel model_;
    RuleEngine engine_;  // в конструкторе: BuildAgentRules()
    int total_ = 0;
    int skipped_ = 0;
    int broken_ = 0;
    bool quiet_ = 0;
    bool ctx_printed_ = 0;
    std::map<std::string, int> cnt;
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H
