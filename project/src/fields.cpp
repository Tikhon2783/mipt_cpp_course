#include "fields.h"

namespace nano_edr {

char c_to_lower(unsigned char c) {
    return std::tolower(c);
}

std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), c_to_lower);
    return result;
}

// nullptr, а не исключение: у file_write нет поля domain, и спрашивать
// про него законно.
const std::string* FindField(const Event& event, const std::string& key) {
    if (key == "ts") {
        return &event.raw_ts();
    }
    if (key == "type") {
        return &event.type();
    }
    if (key == "pid") {
        return &event.pid();
    }
    for (const auto& f : event.fields()) {
        if (f.key == key) {
            return &f.value;
        }
    }
    return nullptr;
}

const std::string& GetRequiredField(const Event& event, const std::string& key) {
    const auto* result = FindField(event, key);
    if (result == nullptr) {
        throw std::invalid_argument("Required field '" + key + "' not found");
    }
    return *result;
}

// bool, а не исключение: поврежденная строка журнала - не нарушение контракта
bool GetIntField(const Event& event, const std::string& key, uint64_t* out) {
    const auto* field = FindField(event, key);
    if (field == nullptr) {
        return false;
    }

    try {
        if (!(*field).empty() && (*field)[0] == '-') {
            return false;
        }
        size_t pos = 0;
        *out = std::stoull(*field, &pos);
        return pos == field->length();
    } catch (const std::exception& e) {
        return false;
    }
}

// bool, а не исключение: отсутствие поля или поврежденное поле - не нарушение контракта
uint64_t GetIntField(const Event& event, const std::string& key, uint64_t fallback) {
    uint64_t result;
    if (GetIntField(event, key, &result)) {
        return result;
    }
    return fallback;
}

// Предикаты ничего не бросают

bool IsProcessStart(const Event& event) {
    return event.type() == "process_start";
}

bool IsFileWrite(const Event& event) {
    return event.type() == "file_write";
}

bool IsNetConnect(const Event& event) {
    return event.type() == "net_connect";
}

bool PathEndsWith(const Event& event, const std::string& suffix) {
    const auto* path_ptr = FindField(event, "path");
    if (path_ptr == nullptr) {
        return false;
    }

    std::string path = NormalizePath(*path_ptr);
    std::string suffix_lower = to_lower(suffix);

    if (path.length() < suffix_lower.length()) {
        return false;
    }

    return path.compare(path.length() - suffix_lower.length(), suffix_lower.length(), suffix_lower) == 0;
}

bool CommandLineContains(const Event& event, const std::string& needle) {
    const auto* cmdline_ptr = FindField(event, "cmdline");
    if (cmdline_ptr == nullptr) {
        return false;
    }

    std::string cmdline_norm = NormalizePath(*cmdline_ptr);
    std::string needle_norm = NormalizePath(needle);

    return cmdline_norm.find(needle_norm) != std::string::npos;
}

std::string NormalizePath(const std::string& path) {
    std::string result = to_lower(path);
    std::replace(result.begin(), result.end(), '/', '\\');
    std::string temp_path = "\\appdata\\local\\temp";

    size_t pos = 0;
    while ((pos = result.find("%temp%", pos)) != std::string::npos) {
        result.replace(pos, 6, temp_path);
        pos += temp_path.length();
    }
    pos = 0;
    while ((pos = result.find("%tmp%", pos)) != std::string::npos) {
        result.replace(pos, 5, temp_path);
        pos += temp_path.length();
    }
    pos = 0;
    while ((pos = result.find("\\\\", pos)) != std::string::npos) {
        result.erase(pos, 1);
    }

    return result;
}

}  // namespace nano_edr
