#ifndef NANO_EDR_AGENT_RULES_H
#define NANO_EDR_AGENT_RULES_H

#include <memory>
#include <vector>

#include "rule.h"

namespace nano_edr {

std::vector<std::unique_ptr<IRule>> BuildAgentRules();

}  // namespace nano_edr

#endif  // NANO_EDR_AGENT_RULES_H
