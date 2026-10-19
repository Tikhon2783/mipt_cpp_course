#include "agent_rules.h"

#include "fields.h"

namespace nano_edr {
    

bool CheckScriptHostFromTemp(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }

    const std::string* image = FindField(event, "image");
    if (!image) {
        throw std::invalid_argument("image is required");
    }

    std::string image_normalized = NormalizePath(*image);

    if (image_normalized.find("wscript.exe") == std::string::npos && image_normalized.find("cscript.exe") == std::string::npos) {
        return false;
    }

    return CommandLineContains(event, "\\appdata\\local\\temp\\") || CommandLineContains(event, "\\windows\\temp\\");
}

bool CheckLolbinDownload(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }

    const std::string* image = FindField(event, "image");
    if (!image) {
        throw std::invalid_argument("image is required");
    }

    std::string image_norm = NormalizePath(*image);

    if (image_norm.find("certutil.exe") == std::string::npos && image_norm.find("bitsadmin.exe") == std::string::npos) {
        return false;
    }

    return CommandLineContains(event, "urlcache") ||
           CommandLineContains(event, "transfer") ||
           CommandLineContains(event, "http:") ||
           CommandLineContains(event, "https:");
}

bool CheckHiddenPowershell(const Event& event) {
    if (!IsProcessStart(event)) {
        return false;
    }

    const std::string* image = FindField(event, "image");
    if (!image) {
        throw std::invalid_argument("image is required");
    }

    std::string image_norm = NormalizePath(*image);

    if (image_norm.find("powershell.exe") == std::string::npos && image_norm.find("pwsh.exe") == std::string::npos) {
        return false;
    }

    return CommandLineContains(event, "-w hidden") ||
           CommandLineContains(event, "-windowstyle hidden") ||
           CommandLineContains(event, "-enc") ||
           CommandLineContains(event, "-encodedcommand");
}

bool CheckAutostartWrite(const Event& event) {
    if (event.type() != "file_create" &&
        event.type() != "file_write" &&
        event.type() != "file_move") {
        return false;
    }

    const std::string* path = nullptr;
    if (event.type() == "file_move") {
        path = FindField(event, "to");
    } else {
        path = FindField(event, "path");
    }

    if (!path) {
        return false;
    }

    std::string path_norm = NormalizePath(*path);
    return path_norm.find("\\start menu\\programs\\startup\\") != std::string::npos;
}

bool CheckRansomExtension(const Event& event) {
    if (event.type() != "file_create" &&
        event.type() != "file_write" &&
        event.type() != "file_move") {
        return false;
    }

    const std::string* path = nullptr;
    if (event.type() == "file_move") {
        path = FindField(event, "to");
    } else {
        path = FindField(event, "path");
    }

    if (!path) {
        return false;
    }

    std::string path_norm = NormalizePath(*path);
    return path_norm.length() > 7 && path_norm.substr(path_norm.length() - 7) == ".locked";
}

// Таблица правил
static constexpr Rule kRules[] = {
    {"script_host_from_temp", CheckScriptHostFromTemp, Severity::kHigh},
    {"lolbin_download", CheckLolbinDownload, Severity::kHigh},
    {"hidden_powershell", CheckHiddenPowershell, Severity::kMedium},
    {"autostart_write", CheckAutostartWrite, Severity::kHigh},
    {"ransom_extension", CheckRansomExtension, Severity::kCritical},
};

const Rule* AgentRules() {
    return kRules;
}

size_t AgentRuleCount() {
    return 5;
}

}  // namespace nano_edr
