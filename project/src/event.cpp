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

    event_type_ = EventTypeFromString(type_);
}

EventType EventTypeFromString(const std::string& type) {
    if (type == "process_start") {
        return EventType::kProcessStart;
    }
    if (type == "process_end") {
        return EventType::kProcessEnd;
    }
    if (type == "file_create") {
        return EventType::kFileCreate;
    }
    if (type == "file_write") {
        return EventType::kFileWrite;
    }
    if (type == "file_delete") {
        return EventType::kFileDelete;
    }
    if (type == "file_move") {
        return EventType::kFileMove;
    }
    if (type == "net_connect") {
        return EventType::kNetConnect;
    }
    return EventType::kOther;
}

const char* EventTypeName(EventType type) {
    switch (type) {
        case EventType::kProcessStart:
            return "process_start";
        case EventType::kProcessEnd:
            return "process_end";
        case EventType::kFileCreate:
            return "file_create";
        case EventType::kFileWrite:
            return "file_write";
        case EventType::kFileDelete:
            return "file_delete";
        case EventType::kFileMove:
            return "file_move";
        case EventType::kNetConnect:
            return "net_connect";
        case EventType::kOther:
            return "other";
    }
    return "other";
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
