#include "event.h"

namespace nano_edr {

Event::Event(const EventParts& parts) : raw_ts_(parts.ts), type_(parts.type), pid_(parts.pid), fields_(parts.fields) {
    if (parts.ts.empty()) {
        throw std::invalid_argument("пустой ts");
    }

    for (char c : parts.ts) {
        if (c < '0' || c > '9') {
            throw std::invalid_argument("неверное время события: " + parts.ts);
        }
    }

    try {
        ts_.ms = std::stoull(parts.ts);
    } catch (const std::out_of_range&) {
        throw std::invalid_argument("ts не помещается в uint64_t: " + parts.ts);
    }

    if (type_.empty()) {
        throw std::invalid_argument("пустой type");
    }
}

std::string ToString(const Event& event) {
    std::string result = "ts=" + event.raw_ts() + " type=" + event.type() + " pid=" + event.pid();
    for (const Field& field : event.fields()) {
        result += ' ';
        result += field.key;
        result += '=';
        result += field.value;
    }
    return result;
}

std::ostream& operator<<(std::ostream& out, const Event& event) {
    return out << ToString(event);
}

}  // namespace nano_edr
