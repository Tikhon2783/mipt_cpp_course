#include <string>

// #include "l1.2/event.h"
#include "event.h"
#include <vector>

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
    if ((*line).empty() || !(*line).empty() && ((*line)[0] == '#' || (*line)[0] == ';')) {
        return true;
    }
    for (size_t i = 0; i < (*line).size(); i++)
    {
        if ((*line)[i] != ' ' && (*line)[i] != '\t')
        {
            return false;
        }
    }
    return true;
}

bool ParseEventLine(const std::string* line, Event* out) {
    if (IsBlankOrComment(line))
    {
        return false;
    }
    const std::string s = *line;

    bool is_key = false;
    bool is_val = false;
    int i_s = 0;
    int i_e = 0;
    bool val_quoted = false;
    std::vector<bool> good(2, false);
    std::vector<Field> fields;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (!is_key && !is_val && (s[i] == ' ' or s[i] == '\t'))
        {
            continue;
        }

        if (!is_key && !is_val)
        {
            if (s[i] == '=')
            {
                return false;
            }
            is_key = true;
            i_s = i;
        }
        
        //   ts=1730000001000 type=file_write pid=1042 path="C:\a b.js" size=812
        if (is_key)
        {
            if (is_key && s[i] == '=')
            {
                is_key = false;
                i_e = i;
            }
            else if (is_key && !('a' <= s[i] && s[i] <='z' || 'A' <= s[i] && s[i] <='Z' || '0' <= s[i] && s[i] <='9'))
            {
                return false;
            }
            continue;
        }
        
        if (!is_val)
        {
            is_val = true;
            if (s[i] == '"')
            {
                val_quoted = true;
            }
            continue;
        }
        if (!val_quoted && !('a' <= s[i] && s[i] <='z' || 'A' <= s[i] && s[i] <='Z' || '0' <= s[i] && s[i] <='9'))
        {
            return false;
        }
        if (val_quoted && s[i] == '"')
        {
            fields.push_back(Field{s.substr(i_s, i_e - i_s), s.substr(i_e + 2, i + 1 - i_e)});
            is_val = false;
            val_quoted = false;
            continue;
        }
        if (val_quoted)
        {
            continue;
        }
        if (s[i] == ' ' or s[i] == '\t')
        {
            fields.push_back(Field{s.substr(i_s, i_e - i_s), s.substr(i_e + 2, i - i_e)});
            is_val = false;
            val_quoted = false;
            continue;
        }
    }
    
    if (is_key)
    {
        return false;
    }
    if (val_quoted && is_val)
    {
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
        return false;
    }

    for (auto f : fields)
    {
        if (f.key == "ts")
        {
            out->ts = f.value;
        }
        else if (f.key == "type")
        {
            out->type = f.value;
        }
        else if (f.key == "pid")
        {
            out->pid = f.value;
        }
        else
        {
            out->fields.push_back(f);
        }
    }
    return true;
}

}
