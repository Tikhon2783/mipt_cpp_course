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
#include "event.h"
#include "event_list.h"
#include "parse.h"

#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <vector>
#include <map>

#include <getopt.h>
// #include <cstdlib>

const bool DBG_MODE = 0;
template <typename... Args>
void debug_print(std::format_string<Args...> format, Args&&... args) {
    if (!DBG_MODE) {
        return;
    }
    std::print(format, std::forward<Args>(args)...);
}

int main(int argc, char** argv) {
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
    //
    int opt, option_index = 0;
    static struct option long_options[] = {
        {"quiet",        no_argument,       nullptr,      'q'},
        {"window-size",  required_argument, nullptr,      'w'},
        {nullptr,       0,                 nullptr,      0},
    };
    while ((opt = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'q': flag_quiet = true; break;
            case 'w': window_size = std::atoi(optarg); break;
            case '?': break;
        }
    }
    //
    
    long long lines = 0;
    long long comments = 0;
    std::string line;
    
    std::vector<std::string> keywords = {"wscript.exe", ".locked", "certutil.exe", "\\Startup\\"};
    std::map<std::string, int> cnt;

    nano_edr::EventList events_window;
    events_window.capacity = window_size;

    while (std::getline(log, line)) {
        // Счётчик увеличивается до всех проверок: он считает строки файла,
        // а не события. Номер, посчитанный по событиям, бесполезен — по нему
        // нельзя открыть файл и посмотреть.
        ++lines;

        // Строки-комментарии в журнале начинаются с '#'
        if (nano_edr::IsBlankOrComment(&line)) {
            ++comments;
            debug_print("[DBG] skip blank/comment\n");
            continue;
        }

        nano_edr::Event event;
        if (!nano_edr::ParseEventLine(&line, &event)) {
            debug_print("[DBG] malformed line, skip\n");
            continue;
        }
        ++cnt[event.pid];

        debug_print("[DBG] line=`{}`\n", line);

        bool keyword_detected = false;
        for (size_t i = 0; i < keywords.size(); i++)
        {
            if (line.find(keywords[i]) != std::string::npos)
            {
                keyword_detected = true;
                std::print("[DETECT] строка {}, признак {}: {}\n", lines, keywords[i], line);
            }
        }
        
        if (keyword_detected && events_window.size > 0 && !flag_quiet)
        {
            debug_print("1\n");
            std::vector<nano_edr::Event*> last_context(std::min(context_size, events_window.size));
            auto head = events_window.head;

            debug_print("2\n");
            for (size_t i = 0; i < events_window.size - last_context.size(); i++)
            {
                head = head->next;
            }
            debug_print("3\n");
            for (size_t i = 0; i < last_context.size(); i++)
            {
                last_context[i] = &(head->event);
                head = head->next;
            }
            debug_print("4\n");
            int _ = std::min((size_t)2, events_window.size);
            debug_print("5\n");
            
            for (size_t j = 0; j < last_context.size(); j++)
            {
                debug_print("6\n");
                debug_print("last_context[j] == nullptr ?: {}", last_context[j] == nullptr);
                std::print("[CTX] -{}: ts={} type={} pid={}\n", last_context.size() - j, last_context[j]->ts, last_context[j]->type, last_context[j]->pid);
            }
        }

        debug_print("[DBG] after detect...\n");
        nano_edr::ListPushBack(&events_window, &event);
    }

    nano_edr::ListClear(&events_window);
    if (!flag_quiet)
    {
        std::print("строк {}, из них комментариев {}\n", lines, comments);
        std::print("всего событий {}, из них:\n", lines - comments);
        for (auto t : cnt)
        {
            std::print("\t{}: {}\n", t.first, t.second);
        }
    }
    
    return 0;
}
