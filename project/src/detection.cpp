#include "detection.h"

#include <format>
#include <string>

namespace nano_edr {

std::string ToString(const Detection& detection) {
    std::string line = std::format(
        "[DETECT] {}  {}  ts={} pid={}",
        SeverityName(detection.severity),
        detection.rule,
        detection.ts.ms,
        detection.pid);
    if (!detection.chain.empty()) {
        line += " chain=";
        line += detection.chain;
    }
    return line;
}

}  // namespace nano_edr
