#ifndef NANO_EDR_AGENT_H
#define NANO_EDR_AGENT_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include "event.h"
#include "event_source.h"
#include "response.h"
#include "ring_buffer.h"
#include "rule_engine.h"
#include "window_entry.h"

namespace nano_edr {

class Agent : public IEventHandler {
 public:
    static constexpr std::size_t kWindowCapacity = 256;

    Agent(bool quiet, uint64_t dedup_period_ms);

    void HandleEvent(const Event& event) override;
    void OnSkipped() override;
    void OnBroken() override;
    void OnTick() override;

    void PrintSummary() const;

    void Subscribe(AlertSink sink);

    void EnableResponse(OsHandle* os, bool dry_run,
                        const std::string& quarantine_dir);

 private:
    void PrintContextOnce();

    RingBuffer<WindowEntry, kWindowCapacity> window_;
    EntityModel model_;
    RuleEngine engine_;  // в конструкторе: BuildAgentRules()
    int total_ = 0;
    int skipped_ = 0;
    int broken_ = 0;
    bool quiet_ = 0;
    bool ctx_printed_ = 0;
    uint64_t dedup_period_ms_{};
    std::size_t suppressed_{};
    std::map<std::string, int> cnt;
    std::unique_ptr<ResponsePlanner> planner_{};
    std::unique_ptr<Responder> responder_{};
};

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_H
