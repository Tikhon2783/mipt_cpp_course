#include <stdexcept>
#include <string>
#include <print>
#include <utility>

#include "alert_sink.h"
#include "dedup_sink.h"
#include "detection.h"

namespace nano_edr {

// --- ConsoleSink -----------------------------------------------------------

void ConsoleSink::operator()(const Detection& detection) const {
    std::print("{}\n", ToString(detection));
}

// --- FileSink --------------------------------------------------------------

FileSink::FileSink(const std::string& path) : out_(path) {
    if (!out_.is_open()) {
        throw std::runtime_error("не удалось открыть файл: " + path);
    }
}

void FileSink::operator()(const Detection& detection) {
    out_ << ToString(detection) << '\n';
}

// --- CountingSink ----------------------------------------------------------

void CountingSink::operator()(const Detection& detection) const {
    ++counter_->total;
    counter_->seen.push_back(SeenAlert{detection.rule, detection.severity,
                                       detection.ts, detection.pid,
                                       detection.actions});
}

// --- FanOutSink ------------------------------------------------------------

void FanOutSink::Add(AlertSink sink) {
    sinks_.push_back(sink);
}

void FanOutSink::operator()(const Detection& detection) const {
    for (auto sink : sinks_)
    {
        sink(detection);
    }
}

// --- DedupSink -------------------------------------------------------------

DedupSink::DedupSink(AlertSink inner, uint64_t period_ms, std::size_t* suppressed)
    : inner_(std::move(inner)), period_ms_(period_ms), suppressed_(suppressed) {}

void DedupSink::operator()(const Detection& detection) {
    if (period_ms_ == 0) {
        inner_(detection);
        return;
    }

    for (Entry& entry : seen_) {
        if (entry.rule == detection.rule && entry.pid == detection.pid) {
            if (detection.ts.ms - entry.first_ts <= period_ms_) {
                if (suppressed_ != nullptr) {
                    ++*suppressed_;
                }
                return;
            }
            entry.first_ts = detection.ts.ms;
            inner_(detection);
            return;
        }
    }

    seen_.push_back(Entry{detection.rule, detection.pid, detection.ts.ms});
    inner_(detection);
}

}  // namespace nano_edr
