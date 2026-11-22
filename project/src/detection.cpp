#include <string>
#include <format>

#include "detection.h"

namespace nano_edr {

std::string ToString(const Detection& detection) {
    return std::format(
        "[DETECT] {}  {}  ts={} pid={}",
        SeverityName(detection.severity),
        detection.rule,
        detection.ts.ms,
        detection.pid
    );
}

}  // namespace nano_edr
