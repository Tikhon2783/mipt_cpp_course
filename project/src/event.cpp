#include "event.h"

#include <stdexcept>
#include <utility>

#include "copy_stats.h"

namespace nano_edr {

Timestamp Event::Validate(const std::string& raw_ts, const std::string& type) {
    if (raw_ts.empty()) {
        throw std::invalid_argument("пустой ts");
    }

    for (char c : raw_ts) {
        if (c < '0' || c > '9') {
            throw std::invalid_argument("неверное время события: " + raw_ts);
        }
    }

    Timestamp result;
    try {
        result.ms = std::stoull(raw_ts);
    } catch (const std::out_of_range&) {
        throw std::invalid_argument("ts не помещается в uint64_t: " + raw_ts);
    }

    if (type.empty()) {
        throw std::invalid_argument("пустой type");
    }

    return result;
}

Event::Event(const EventParts& parts) {
    ts_ = Validate(parts.ts, parts.type);  // бросает, если ts или type неверны
    event_type_ = EventTypeFromString(parts.type);
    raw_ts_ = parts.ts;
    type_ = parts.type;
    pid_ = parts.pid;
    fields_ = parts.fields;
    copy_stats().event_strings_copied += 3 + 2 * parts.fields.size();
}

Event::Event(EventParts&& parts) {
    ts_ = Validate(parts.ts, parts.type);  // бросает, если ts или type неверны
    // до перемещения вычислить event_type_ из parts.type
    event_type_ = EventTypeFromString(parts.type);
    // число строк считается до std::move(parts.fields)
    const std::size_t strings = 3 + 2 * parts.fields.size();
    // потом забрать строки
    raw_ts_ = std::move(parts.ts);
    type_ = std::move(parts.type);
    pid_ = std::move(parts.pid);
    fields_ = std::move(parts.fields);
    copy_stats().event_strings_moved += strings;
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
