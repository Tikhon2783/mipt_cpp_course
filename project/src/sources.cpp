#include "sources.h"

#include <print>

#include "event.h"
#include "parse.h"

namespace nano_edr {

FileSource::FileSource(const std::string& path) : log_(path) {
    if (!log_) {
        throw std::runtime_error("не удалось открыть журнал: " + path);
    }
}

void FileSource::Run(IEventHandler* handler) {
    std::string line;
    while (std::getline(log_, line)) {
        if (IsBlankOrComment(line)) {
            handler->OnSkipped();
            continue;
        }

        EventParts parts;
        if (!ParseEventParts(line, &parts)) {
            handler->OnBroken();
            continue;
        }

        try {
            Event event(parts);
            handler->HandleEvent(event);
        } catch (const std::exception& e) {
        }
    }
}

OsSource::OsSource(const std::string& config_path) : handle_(config_path) {}

void OsSource::Run(IEventHandler* handler) {
    handle_.Subscribe(&OsSource::Trampoline, handler);
    handle_.Start();

    os_status status = handle_.Wait(1000);
    while (status == OS_TIMEOUT) {
        handler->OnTick();
        status = handle_.Wait(1000);
    }

    if (status != OS_OK) {
        throw std::runtime_error(std::string("os_wait: ") + os_status_str(status));
    }
}

void OsSource::Trampoline(const os_event* ev, void* ctx) noexcept {
    try {
        EventParts parts;
        parts.ts = std::to_string(ev->ts);
        if (ev->type != nullptr) {
            parts.type = ev->type;
        }
        if (ev->pid != 0) {
            parts.pid = std::to_string(ev->pid);
        }
        for (std::size_t i = 0; i < ev->field_count; i++) {
            parts.fields.push_back(Field{ev->fields[i].key, ev->fields[i].value});
        }

        Event event(parts);
        static_cast<IEventHandler*>(ctx)->HandleEvent(event);
    } catch (const std::exception& e) {
        std::print(stderr, "пропущено событие: {}\n", e.what());
    } catch (...) {
        std::print(stderr, "пропущено событие: неизвестная ошибка\n");
    }
}

}  // namespace nano_edr
