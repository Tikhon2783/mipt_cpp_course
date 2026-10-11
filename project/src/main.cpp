// Каркас агента: читает журнал событий построчно и считает строки.
//
// Это заготовка занятия 1.1, а не решение. Детектов она не ищет — их вы
// добавите здесь же, в отмеченном месте ниже. Формат строки детекта, список
// признаков и правило про их порядок заданы в постановке занятия: по ним
// сравниваются эталоны.
//
// Весь код лежит в main, и на этом занятии так и надо: функции появятся
// на занятии 1.2, ссылки — на 1.3. Разбор аргументов, коды возврата и флаг
// --quiet — часть задания.
//
// Запуск:
//   nano-edr <журнал.log>
#include <cstdio>
#include <fstream>
#include <map>
#include <print>
#include <string>
#include <vector>

#include "event.h"
#include "event_list.h"
#include "parse.h"
#include "rules.h"
#include "agent_rules.h"

constexpr bool DBG_MODE = 0;
template <typename... Args>
void debug_print(std::format_string<Args...> format, Args&&... args) {
    if (!DBG_MODE) {
        return;
    }
    std::print(format, std::forward<Args>(args)...);
}



struct Stats {
    bool flag_quiet = false;
    long long lines;
    long long comments;
    std::map<std::string, int> cnt;
    Stats() : lines(0), comments(0), cnt(std::map<std::string, int>()) {}

    ~Stats() {
        if (!flag_quiet) {
            std::print("строк {}, из них комментариев {}\n", lines, comments);
            std::print("всего событий {}, из них:\n", lines - comments);
            for (auto t : cnt) {
                std::print("\t{}: {}\n", t.first, t.second);
            }
        }
    }
};

int main(int argc, char** argv) {
    try {
        // Аргументы разбираются грубо: путь к журналу и ничего больше. Остальное,
        // включая --quiet, добавляется по заданию.
        if (argc < 2) {
            std::print(stderr, "использование: nano-edr <журнал.log>\n");
            return 2;
        }
    
        std::ifstream log(argv[1]);
        if (!log) {
            std::print(stderr, "не удалось открыть журнал: {}\n", argv[1]);
            return 2;
        }
    
        bool flag_quiet = false;
        int window_size = 64;
        size_t context_size = 2;
        const std::vector<std::string> keywords = {"wscript.exe", ".locked", "certutil.exe", "\\Startup\\"};
    
        for (int i = 1; i < argc; i++) {
            if (std::string(argv[i]) == "--quiet") {
                debug_print("QUIET\n");
                flag_quiet = true;
            } else if (std::string(argv[i]) == "--window-size") {
                if (i + 1 < argc) {
                    window_size = std::atoi(argv[++i]);
                } else {
                    throw std::invalid_argument("с --window-size нужен аргумент N");
                }
            }
        }
    
        Stats stats = Stats();
        nano_edr::EventList events_window;
        events_window.capacity = window_size;
        std::string line;
    
        while (std::getline(log, line)) {
            // Счётчик увеличивается до всех проверок: он считает строки файла,
            // а не события. Номер, посчитанный по событиям, бесполезен — по нему
            // нельзя открыть файл и посмотреть.
            ++stats.lines;
    
            // Строки-комментарии в журнале начинаются с '#'
            if (nano_edr::IsBlankOrComment(&line)) {
                ++stats.comments;
                debug_print("[DBG] skip blank/comment\n");
                continue;
            }
    
            nano_edr::Event event;
            if (!nano_edr::ParseEventLine(&line, &event)) {
                debug_print("[DBG] malformed line, skip\n");
                continue;
            }
            ++stats.cnt[event.pid];
    
            debug_print("[DBG] line=`{}`\n", line);
    
            auto rules_detected = nano_edr::CheckRules(event, nano_edr::AgentRules(), nano_edr::AgentRuleCount());
    
            if (rules_detected != 0 && !flag_quiet) {
                auto curr_context_size = std::min(context_size, events_window.size);
                auto context_head = events_window.head;
                for (size_t i = 0; i < events_window.size - curr_context_size; i++) {
                    context_head = context_head->next;
                }
    
                for (size_t j = 0; j < curr_context_size; j++) {
                    if (context_head == nullptr) {
                        break;
                    }
                    std::print("[CTX] -{}: ts={} type={} pid={}\n", curr_context_size - j, context_head->event.ts, context_head->event.type, context_head->event.pid);
                    context_head = context_head->next;
                }
            }
    
            nano_edr::ListPushBack(&events_window, &event);
        }
        //
    } catch (const std::exception& e) {
        std::print("ошибка: {}\n", e.what());
        return 1;
    }

    return 0;
}
