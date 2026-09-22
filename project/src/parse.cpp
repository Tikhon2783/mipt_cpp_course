#include <string>

// #include "l1.2/event.h"
#include "event.h"
#include <vector>

#include <print>

// struct Field {
//     std::string key;
//     std::string value;
// };
// struct Event {
//     std::string ts;    // время в миллисекундах, как в журнале
//     std::string type;  // process_start, file_write, net_connect, …
//     std::string pid;

//     std::vector<Field> fields;  // всё, кроме ts, type и pid
// };

namespace nano_edr {

bool IsBlankOrComment(const std::string* line) {
    for (size_t i = 0; i < (*line).size(); i++)
    {
        if ((*line)[i] == ';' || (*line)[i] == '#') {
            return true;
        }
        if ((*line)[i] != ' ' && (*line)[i] != '\t')
        {
            return false;
        }
    }
    return true;
}

bool valid_symbol(char c) {
    std::string specchar = "_=";
    return ('a' <= c && c <='z' || 'A' <= c && c <='Z' || '0' <= c && c <='9' || specchar.find(c) != std::string::npos);
}

bool ParseEventLine(const std::string* line, Event* out) {
    if (IsBlankOrComment(line))
    {
        std::print("ret 0\n");
        return false;
    }
    const std::string s = *line;

    bool is_key = false;
    bool is_val = false;
    int i_s = 0;
    int i_e = 0;
    bool val_quoted = false;
    std::vector<bool> good(3, false);
    std::vector<Field> fields;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (!is_key && !is_val && (s[i] == ' ' || s[i] == '\t'))
        {
            std::print("skip space\n");
            continue;
        }

        if (!is_key && !is_val)
        {
            if (s[i] == '=')
            {
                std::print("ret 1\n");
                return false;
            }
            std::print("was '{}'\n", s[i]);
            is_key = true;
            i_s = i;
        }
        
        //   ts=1730000001000 type=file_write pid=1042 path="C:\a b.js" size=812
        if (is_key)
        {
            if (s[i] == '=')
            {
                is_key = false;
                i_e = i;
            }
            else if (!valid_symbol(s[i]))
            {
                std::print("ret 2\n");
                std::print("line = '{}'\ni = {}, s[i] = '{}', substr = '{}'\n", s, i, s[i], s.substr(0, i + 1));
                std::print("ord(s[i]) = {}\n", static_cast<int>(s[i]));
                std::print("is_key={}, is_val={}\n", is_key, is_val);
                return false;
            }
            else
            {
                continue;
            }
        }
        
        if (!is_val)
        {
            is_val = true;
            if (i + 1 < s.size() && s[i + 1] == '"')
            {
                val_quoted = true;
                ++i;
            }
            continue;
        }
        if (!val_quoted && (s[i] == ' ' || s[i] == '\t'))
        {
            fields.push_back(Field{s.substr(i_s, i_e - i_s), s.substr(i_e + 1, i - i_e - 1)});
            is_val = false;
            val_quoted = false;
            continue;
        }
        if (!val_quoted && !valid_symbol(s[i]))
        {
            std::print("ret 3\n");
            std::print("line = '{}'\ni = {}, s[i] = '{}', substr = '{}'\n", s, i, s[i], s.substr(0, i + 1));
            std::print("ord(s[i]) = {}\n", static_cast<int>(s[i]));
            std::print("is_key={}, is_val={}\n", is_key, is_val);
            return false;
        }
        if (val_quoted && s[i] == '"')
        {
            fields.push_back(Field{s.substr(i_s, i_e - i_s), s.substr((i_e + 1) + 1, i - (i_e + 1) - 1)});
            is_val = false;
            val_quoted = false;
            continue;
        }
        if (val_quoted)
        {
            continue;
        }
    }
    if (is_val && !val_quoted)
    {
        fields.push_back(Field{s.substr(i_s, i_e - i_s), s.substr(i_e + 1, s.size() - i_e - 1)});
    }

    
    if (is_key)
    {
        std::print("ret 4\n");
        return false;
    }
    if (val_quoted && is_val)
    {
        std::print("ret 5\n");
        return false;
    }
    for (auto f : fields)
    {
        if (f.key == "ts")
        {
            good[0] = true;
        }
        else if (f.key == "type")
        {
            good[1] = true;
        }
    }
    if (!good[0] || !good[1])
    {
        std::print("ret 6\n");
        return false;
    }

    good[2] = true;
    for (auto f : fields)
    {
        if (f.key == "ts" && good[0])
        {
            out->ts = f.value;
            good[0] = false;
        }
        else if (f.key == "type" && good[1])
        {
            out->type = f.value;
            good[1] = false;
        }
        else if (f.key == "pid" && good[2])
        {
            out->pid = f.value;
            good[2] = false;
        }
        else
        {
            out->fields.push_back(f);
        }
    }
    return true;
}

}
