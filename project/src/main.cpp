#include <memory>
#include <print>

#include "agent.h"
#include "sources.h"
#include "edr_handle.h"

struct Options {
    std::string path;
    std::string out;
    bool quiet = false;
    bool has_config = false;
    bool has_file = false;
};

int main(int argc, char** argv) {
    Options o = Options();

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--quiet") {
            o.quiet = true;
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
        std::print(stderr, "использование: nano-edr [--file] <сценарий.cfg | журнал.log> [--out файл_детектов] [--quiet]\n");
        return 2;
    }

    try {
        nano_edr::EdrHandle edr(o.path);
        nano_edr::Agent agent(o.quiet);
        nano_edr::FanOutSink targets;

        targets.Add(nano_edr::ConsoleSink());
        if (!o.out.empty()) {
            targets.Add(nano_edr::FileSink(o.out));
        }
        targets.Add(nano_edr::EdrSink(&edr));
        agent.Subscribe(std::move(targets));

        std::unique_ptr<nano_edr::IEventSource> source;
        if (o.has_file) {
            source = std::make_unique<nano_edr::FileSource>(o.path);
        } else {
            source = std::make_unique<nano_edr::OsSource>(o.path);
        }
        source->Run(&agent);
        
        if (edr.Flush() != EDR_OK) {
            std::print(stderr, "[WARN] детекты не отправлены\n");
        }
        agent.PrintSummary();
    } catch (const std::exception& e) {
        std::print("необработанное исключение: {}\n", e.what());
        return 1;
    }

    return 0;
}
