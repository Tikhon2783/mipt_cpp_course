#include <print>

#include "agent.h"
#include "sources.h"

int main(int argc, char** argv) {
    bool flag_quiet = false;
    size_t window_size = 64;
    std::string path;
    bool has_config = false;
    bool has_file = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--quiet") {
            flag_quiet = true;
        } else if (arg == "--window-size") {
            if (i + 1 >= argc) {
                std::print(stderr, "с --window-size нужен аргумент N\n");
                return 2;
            }
            window_size = std::atoi(argv[++i]);
        } else if (arg == "--file") {
            if (i + 1 >= argc) {
                std::print(stderr, "с --file нужен аргумент\n");
                return 2;
            }
            if (has_file || has_config) {
                std::print(stderr, "путь задан дважды\n");
                return 2;
            }
            path = argv[++i];
            has_file = true;
        } else if (arg.starts_with("--")) {
            std::print(stderr, "неизвестная опция: {}\n", arg);
            return 2;
        } else {
            if (has_config || has_file) {
                std::print(stderr, "путь задан дважды\n");
                return 2;
            }
            path = arg;
            has_config = true;
        }
    }

    if (!has_file && !has_config) {
        std::print(stderr, "использование: nano-edr <сценарий.cfg | журнал.log>\n");
        return 2;
    }

    try {
        nano_edr::Agent agent(window_size, flag_quiet);
        if (has_file) {
            nano_edr::FileSource source(path);
            source.Run(&agent);
        } else {
            nano_edr::OsSource source(path);
            source.Run(&agent);
        }
        agent.PrintSummary();
    } catch (const std::exception& e) {
        std::print("необработанное исключение: {}\n", e.what());
        return 1;
    }

    return 0;
}
