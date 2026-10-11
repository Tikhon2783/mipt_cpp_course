#include "sources.h"

#include "event.h"
#include "parse.h"

namespace nano_edr {

FileSource::FileSource(const std::string& path) : log_(path) {
    if (!log_) {
        throw std::runtime_error("не удалось открыть журнал: " + path);
    }
}

void FileSource::Run(Agent* agent) {
    std::string line;
    while (std::getline(log_, line)) {
        if (IsBlankOrComment(line)) {
            continue;
        }

        EventParts parts;
        if (!ParseEventParts(line, &parts)) {
            continue;
        }

        try {
            Event event(parts);
            agent->HandleEvent(event);
        } catch (const std::exception& e) {
        }
    }
}

OsSource::OsSource(const std::string& config_path) : handle_(config_path) {}

void OsSource::Run(Agent* agent) {
    handle_.Subscribe(&Agent::Trampoline, agent);
    handle_.Start();

    os_status status = handle_.Wait(1000);
    while (status == OS_TIMEOUT) {
        status = handle_.Wait(1000);
    }

    if (status != OS_OK) {
        throw std::runtime_error(std::string("os_wait: ") + os_status_str(status));
    }
}

}  // namespace nano_edr
