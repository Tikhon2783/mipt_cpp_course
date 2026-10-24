#include "agent_rules.h"

#include <cstdint>
#include <memory>
#include <vector>

#include "conditions.h"

namespace nano_edr {

constexpr std::uint64_t kSequenceWindowMs = 2000;

std::vector<std::unique_ptr<IRule>> BuildAgentRules() {
    std::vector<std::unique_ptr<IRule>> rules;

    // CheckScriptHostFromTemp
    auto script_host = std::make_unique<MatchRule>("script_host_from_temp", Severity::kHigh);
    script_host->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
    {
        auto image = std::make_unique<AnyOf>();
        image->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
        image->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
        script_host->AddCondition(std::move(image));
    }
    {
        auto cmdline = std::make_unique<AnyOf>();
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "\\appdata\\local\\temp\\"));
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "/appdata/local/temp/"));
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "\\windows\\temp\\"));
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "/windows/temp/"));
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "%temp%"));
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "%tmp%"));
        script_host->AddCondition(std::move(cmdline));
    }
    script_host->AddAction("kill_process");
    rules.push_back(std::move(script_host));

    // CheckLolbinDownload
    auto lolbin = std::make_unique<MatchRule>("lolbin_download", Severity::kHigh);
    lolbin->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
    {
        auto image = std::make_unique<AnyOf>();
        image->Add(std::make_unique<FieldContains>("image", "certutil.exe"));
        image->Add(std::make_unique<FieldContains>("image", "bitsadmin.exe"));
        lolbin->AddCondition(std::move(image));
    }
    {
        auto cmdline = std::make_unique<AnyOf>();
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "urlcache"));
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "transfer"));
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "http:"));
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "https:"));
        lolbin->AddCondition(std::move(cmdline));
    }
    lolbin->AddAction("kill_process");
    rules.push_back(std::move(lolbin));

    // CheckHiddenPowershell
    auto powershell = std::make_unique<MatchRule>("hidden_powershell", Severity::kMedium);
    powershell->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
    {
        auto image = std::make_unique<AnyOf>();
        image->Add(std::make_unique<FieldContains>("image", "powershell.exe"));
        image->Add(std::make_unique<FieldContains>("image", "pwsh.exe"));
        powershell->AddCondition(std::move(image));
    }
    {
        auto cmdline = std::make_unique<AnyOf>();
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "-w hidden"));
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "-windowstyle hidden"));
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "-enc"));
        cmdline->Add(std::make_unique<FieldContains>("cmdline", "-encodedcommand"));
        powershell->AddCondition(std::move(cmdline));
    }
    powershell->AddAction("kill_process");
    rules.push_back(std::move(powershell));

    // CheckAutostartWrite
    auto autostart = std::make_unique<MatchRule>("autostart_write", Severity::kHigh);
    {
        auto by_path = std::make_unique<AllOf>();
        by_path->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite}));
        auto path = std::make_unique<AnyOf>();
        path->Add(std::make_unique<FieldContains>("path", "\\start menu\\programs\\startup\\"));
        path->Add(std::make_unique<FieldContains>("path", "/start menu/programs/startup/"));
        by_path->Add(std::move(path));

        auto by_to = std::make_unique<AllOf>();
        by_to->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileMove}));
        auto to = std::make_unique<AnyOf>();
        to->Add(std::make_unique<FieldContains>("to", "\\start menu\\programs\\startup\\"));
        to->Add(std::make_unique<FieldContains>("to", "/start menu/programs/startup/"));
        by_to->Add(std::move(to));

        auto either = std::make_unique<AnyOf>();
        either->Add(std::move(by_path));
        either->Add(std::move(by_to));
        autostart->AddCondition(std::move(either));
    }
    autostart->AddAction("quarantine_file");
    rules.push_back(std::move(autostart));

    // CheckRansomExtension
    auto ransom = std::make_unique<MatchRule>("ransom_extension", Severity::kCritical);
    {
        auto by_path = std::make_unique<AllOf>();
        by_path->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite}));
        by_path->Add(std::make_unique<FieldEndsWith>("path", ".locked"));

        auto by_to = std::make_unique<AllOf>();
        by_to->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileMove}));
        by_to->Add(std::make_unique<FieldEndsWith>("to", ".locked"));

        auto either = std::make_unique<AnyOf>();
        either->Add(std::move(by_path));
        either->Add(std::move(by_to));
        ransom->AddCondition(std::move(either));
    }
    ransom->AddAction("quarantine_file");
    rules.push_back(std::move(ransom));

    // CheckScriptHostNetwork
    auto host_network = std::make_unique<SequenceRule>("script_host_network", Severity::kHigh, kSequenceWindowMs);
    {
        auto start = std::make_unique<AllOf>();
        start->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
        auto host = std::make_unique<AnyOf>();
        host->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
        host->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
        start->Add(std::move(host));
        host_network->SetFirst(std::move(start));
    }
    host_network->SetSecond(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kNetConnect}));
    host_network->AddAction("kill_process");
    rules.push_back(std::move(host_network));

    return rules;
}

}  // namespace nano_edr
