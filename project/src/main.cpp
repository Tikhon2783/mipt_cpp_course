#include <cstdint>
#include <memory>
#include <print>

#include "agent.h"
#include "copy_stats.h"
#include "edr_handle.h"
#include "field_traits.h"
#include "sources.h"

struct Options {
    std::string path;
    std::string out;
    bool quiet = false;
    bool dry_run = false;
    bool copies = false;
    bool has_config = false;
    bool has_file = false;
};

int main(int argc, char** argv) {
    Options o = Options();

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--quiet") {
            o.quiet = true;
        } else if (arg == "--dry-run") {
            o.dry_run = true;
        } else if (arg == "--copies") {
            o.copies = true;
        } else if (arg == "--file") {
            if (i + 1 >= argc) {
                std::print(stderr, "с --file нужен аргумент\n");
                return 2;
            }
            if (o.has_file || o.has_config) {
                std::print(stderr, "путь задан дважды\n");
                return 2;
            }
            o.path = argv[++i];
            o.has_file = true;
        } else if (arg == "--out") {
            if (i + 1 >= argc) {
                std::print(stderr, "с --out нужен аргумент\n");
                return 2;
            }
            o.out = argv[++i];
        } else if (arg.starts_with("--")) {
            std::print(stderr, "неизвестная опция: {}\n", arg);
            return 2;
        } else {
            if (o.has_config || o.has_file) {
                std::print(stderr, "путь задан дважды\n");
                return 2;
            }
            o.path = arg;
            o.has_config = true;
        }
    }

    if (!o.has_file && !o.has_config) {
        std::print(stderr, "использование: nano-edr [--file] <сценарий.cfg | журнал.log> [--out файл_детектов] [--quiet] [--dry-run] [--copies]\n");
        return 2;
    }

    try {
        nano_edr::EdrHandle edr(o.path);

        const std::string text = edr.Config("dedup_period_ms", "10000");
        uint64_t dedup_period_ms = 0;
        if (!nano_edr::FieldTraits<uint64_t>::Parse(text, &dedup_period_ms)) {
            std::print(stderr, "[WARN] dedup_period_ms не разбирается: {}\n", text);
            dedup_period_ms = 10000;
        }

        std::unique_ptr<nano_edr::IEventSource> source;
        if (o.has_file) {
            source = std::make_unique<nano_edr::FileSource>(o.path);
        } else {
            source = std::make_unique<nano_edr::OsSource>(o.path);
        }

        nano_edr::Agent agent(o.quiet, dedup_period_ms);
        nano_edr::FanOutSink targets;

        targets.Add(nano_edr::ConsoleSink());
        if (!o.out.empty()) {
            targets.Add(nano_edr::FileSink(o.out));
        }
        targets.Add(nano_edr::EdrSink(&edr));
        agent.Subscribe(std::move(targets));
        agent.EnableResponse(source->os(), o.dry_run,
                             edr.Config("quarantine_dir", "C:\\quarantine"));

        source->Run(&agent);

        if (edr.Flush() != EDR_OK) {
            std::print(stderr, "[WARN] детекты не отправлены\n");
        }
        agent.PrintSummary();

        if (o.copies) {
            const nano_edr::CopyStats& stats = nano_edr::copy_stats();
            std::print("[COPY] строк события скопировано {}, перемещено {}\n",
                       stats.event_strings_copied, stats.event_strings_moved);
            std::print("[COPY] записей WindowEntry скопировано {}, перемещено {}\n",
                       stats.window_copied, stats.window_moved);
            std::print("[COPY] событий скопировано в детекты {}\n",
                       stats.evidence_copied);
            std::print("[COPY] копий всего {}\n", stats.total());
        }
    } catch (const std::exception& e) {
        std::print("необработанное исключение: {}\n", e.what());
        return 1;
    }

    return 0;
}
