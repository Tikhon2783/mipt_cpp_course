#include "agent_rules.h"

#include <memory>
#include <vector>

#include "conditions.h"

namespace nano_edr {

RuleEngine BuildAgentRules() {
    RuleEngine engine;

    // CheckScriptHostFromTemp
    auto rule_script_host = std::make_unique<MatchRule>("script_host_from_temp", Severity::kHigh);
    rule_script_host->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
    auto host = std::make_unique<AnyOf>();
    host->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    host->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
    rule_script_host->AddCondition(std::move(host));
    auto temp = std::make_unique<AnyOf>();
    temp->Add(std::make_unique<FieldContains>("cmdline", "\\appdata\\local\\temp\\"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "/appdata/local/temp/"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "\\windows\\temp\\"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "/windows/temp/"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "%temp%"));
    temp->Add(std::make_unique<FieldContains>("cmdline", "%tmp%"));
    rule_script_host->AddCondition(std::move(temp));
    rule_script_host->AddAction("kill_process");
    engine.AddRule(std::move(rule_script_host));

    // CheckLolbinDownload
    auto rule_lolbin = std::make_unique<MatchRule>("lolbin_download", Severity::kHigh);
    rule_lolbin->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
    auto lolbin_image = std::make_unique<AnyOf>();
    lolbin_image->Add(std::make_unique<FieldContains>("image", "certutil.exe"));
    lolbin_image->Add(std::make_unique<FieldContains>("image", "bitsadmin.exe"));
    rule_lolbin->AddCondition(std::move(lolbin_image));
    auto lolbin_cmdline = std::make_unique<AnyOf>();
    lolbin_cmdline->Add(std::make_unique<FieldContains>("cmdline", "urlcache"));
    lolbin_cmdline->Add(std::make_unique<FieldContains>("cmdline", "transfer"));
    lolbin_cmdline->Add(std::make_unique<FieldContains>("cmdline", "http:"));
    lolbin_cmdline->Add(std::make_unique<FieldContains>("cmdline", "https:"));
    rule_lolbin->AddCondition(std::move(lolbin_cmdline));
    rule_lolbin->AddAction("kill_process");
    engine.AddRule(std::move(rule_lolbin));

    // CheckHiddenPowershell
    auto rule_powershell = std::make_unique<MatchRule>("hidden_powershell", Severity::kMedium);
    rule_powershell->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
    auto ps_image = std::make_unique<AnyOf>();
    ps_image->Add(std::make_unique<FieldContains>("image", "powershell.exe"));
    ps_image->Add(std::make_unique<FieldContains>("image", "pwsh.exe"));
    rule_powershell->AddCondition(std::move(ps_image));
    auto ps_cmdline = std::make_unique<AnyOf>();
    ps_cmdline->Add(std::make_unique<FieldContains>("cmdline", "-w hidden"));
    ps_cmdline->Add(std::make_unique<FieldContains>("cmdline", "-windowstyle hidden"));
    ps_cmdline->Add(std::make_unique<FieldContains>("cmdline", "-enc"));
    ps_cmdline->Add(std::make_unique<FieldContains>("cmdline", "-encodedcommand"));
    rule_powershell->AddCondition(std::move(ps_cmdline));
    rule_powershell->AddAction("kill_process");
    engine.AddRule(std::move(rule_powershell));

    // CheckAutostartWrite
    auto rule_autostart = std::make_unique<MatchRule>("autostart_write", Severity::kHigh);
    rule_autostart->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite, EventType::kFileMove}));
    auto paths_autostart = std::make_unique<AnyOf>();
    paths_autostart->Add(std::make_unique<FieldContains>("path", "\\start menu\\programs\\startup\\"));
    paths_autostart->Add(std::make_unique<FieldContains>("path", "/start menu/programs/startup/"));
    paths_autostart->Add(std::make_unique<FieldContains>("to", "\\start menu\\programs\\startup\\"));
    paths_autostart->Add(std::make_unique<FieldContains>("to", "/start menu/programs/startup/"));
    rule_autostart->AddCondition(std::move(paths_autostart));
    rule_autostart->AddAction("quarantine_file");
    engine.AddRule(std::move(rule_autostart));

    // CheckRansomExtension
    auto rule_ransom = std::make_unique<MatchRule>("ransom_extension", Severity::kCritical);
    rule_ransom->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite, EventType::kFileMove}));
    auto paths_ransom = std::make_unique<AnyOf>();
    paths_ransom->Add(std::make_unique<FieldEndsWith>("path", ".locked"));
    paths_ransom->Add(std::make_unique<FieldEndsWith>("to", ".locked"));
    rule_ransom->AddCondition(std::move(paths_ransom));
    rule_ransom->AddAction("quarantine_file");
    engine.AddRule(std::move(rule_ransom));

    // beacon
    auto rule_beacon = std::make_unique<SequenceRule>("script_host_beacon", Severity::kCritical, 10000);
    auto start = std::make_unique<AllOf>();
    start->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
    auto beacon_host = std::make_unique<AnyOf>();
    beacon_host->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    beacon_host->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
    start->Add(std::move(beacon_host));
    auto beacon_temp = std::make_unique<AnyOf>();
    beacon_temp->Add(std::make_unique<FieldContains>("cmdline", "\\appdata\\local\\temp\\"));
    beacon_temp->Add(std::make_unique<FieldContains>("cmdline", "/appdata/local/temp/"));
    beacon_temp->Add(std::make_unique<FieldContains>("cmdline", "\\windows\\temp\\"));
    beacon_temp->Add(std::make_unique<FieldContains>("cmdline", "/windows/temp/"));
    beacon_temp->Add(std::make_unique<FieldContains>("cmdline", "%temp%"));
    beacon_temp->Add(std::make_unique<FieldContains>("cmdline", "%tmp%"));
    start->Add(std::move(beacon_temp));
    rule_beacon->SetFirst(std::move(start));
    rule_beacon->SetSecond(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kNetConnect}));
    rule_beacon->AddAction("kill_process");
    engine.AddRule(std::move(rule_beacon));

    return engine;
}

}  // namespace nano_edr
