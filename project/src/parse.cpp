#include <string>

// #include "l1.2/event.h"
#include "event.h"
#include <vector>

namespace nano_edr {

bool IsBlankOrComment(const std::string* line) {
    size_t p = line->find_first_not_of(" \t");
    if (p == std::string::npos || (*line)[p] == ';' || (*line)[p] == '#')
    {
        return true;
    }
    return false;
}

bool isSpace(char c) {
    return c == ' ' || c == '\t';
}

bool ParseEventLine(const std::string* line, Event* out) {
    if (IsBlankOrComment(line))
    {
        return false;
    }

    bool is_key = false;
    bool is_val = false;
    size_t i_s = 0;
    size_t i_e = 0;
    bool val_quoted = false;
    std::vector<Field> fields;
    for (size_t i = 0; i < (*line).size(); i++)
    {
        if (!is_key && !is_val && (isSpace((*line)[i])))
        {
            continue;
        }
        
        if (!is_key && !is_val)
        {
            if ((*line)[i] == '=')
            {
                return false;
            }
            is_key = true;
            i_s = i;
        }
        
        // парсинг/валидация ключа
        if (is_key)
        {
            if ((*line)[i] == '=')
            {
                is_key = false;
                i_e = i;
            }
            else if (isSpace((*line)[i]))
            {
                return false;
            }
            else
            {
                continue;
            }
        }
        
        // парсинг/валидация значения
        if (!is_val)
        {
            is_val = true;
            if (i + 1 < (*line).size() && (*line)[i + 1] == '"')
            {
                val_quoted = true;
                ++i;
            }
            continue;
        }
        // конец значения не в кавычках
        if (!val_quoted && (isSpace((*line)[i])))
        {
            fields.push_back(Field{(*line).substr(i_s, i_e - i_s), (*line).substr(i_e + 1, i - i_e - 1)});
            is_val = false;
            val_quoted = false;
            continue;
        }
        // конец значения в кавычках
        if (val_quoted && (*line)[i] == '"')
        {
            if (i != (*line).size() - 1 && !isSpace((*line)[i + 1]))
            {
                return false;
            }
            fields.push_back(Field{(*line).substr(i_s, i_e - i_s), (*line).substr((i_e + 1) + 1, i - (i_e + 1) - 1)});
            is_val = false;
            val_quoted = false;
            continue;
        }
    }
    if (is_val && !val_quoted)
    {
        fields.push_back(Field{(*line).substr(i_s, i_e - i_s), (*line).substr(i_e + 1, (*line).size() - i_e - 1)});
    }

    
    if (is_key)
    {
        return false;
    }
    if (val_quoted && is_val)
    {
        return false;
    }
    int spec_keys_exist = 0b00;
    for (const Field& f : fields)
    {
        if (f.key == "ts")
        {
            spec_keys_exist |= 0b01;
        }
        else if (f.key == "type")
        {
            spec_keys_exist |= 0b10;
        }
    }
    if (spec_keys_exist != 0b11)
    {
        return false;
    }
    
    int spec_keys_unused = 0b111;
    for (const Field& f : fields)
    {
        if (f.key == "ts" && (spec_keys_unused & 0b100))
        {
            out->ts = f.value;
            spec_keys_unused &= 0b011;
        }
        else if (f.key == "type" && (spec_keys_unused & 0b010))
        {
            out->type = f.value;
            spec_keys_unused &= 0b101;
        }
        else if (f.key == "pid" && (spec_keys_unused & 0b001))
        {
            out->pid = f.value;
            spec_keys_unused &= 0b110;
        }
        else
        {
            out->fields.push_back(f);
        }
    }
    return true;
}

}
