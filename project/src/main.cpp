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
#include <print>
#include <string>
#include <vector>
#include <map>

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
    for (size_t i = 0; i < argc; i++)
    {
        if (std::string(argv[i]) == "--quiet")
        {
            flag_quiet = true;
        }
    }
    

    long long lines = 0;
    long long comments = 0;
    std::string line;
    
    std::vector<std::string> keywords = {"wscript.exe", ".locked", "certutil.exe", "\\Startup\\"};
    std::map<std::string, int> cnt;

    while (std::getline(log, line)) {
        // Счётчик увеличивается до всех проверок: он считает строки файла,
        // а не события. Номер, посчитанный по событиям, бесполезен — по нему
        // нельзя открыть файл и посмотреть.
        ++lines;

        // Строки-комментарии в журнале начинаются с '#'. Они не события,
        // и детекта по ним быть не должно.
        if (!line.empty() && line[0] == '#') {
            ++comments;
            continue;
        }

        // >>> Здесь начинается занятие 1.1.
        //
        // Проверка признаков и печать детекта. Номер строки, который нужен
        // в выводе, — это lines.
        for (size_t i = 0; i < keywords.size(); i++)
        {
            ++cnt[line.substr(22, line.find(" pid=") - 22)];
            if (line.find(keywords[i]) != std::string::npos)
            {
                std::print("[DETECT] строка {}, признак {}: {}\n", lines, keywords[i], line);
            }
        }
        
    }

    
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
