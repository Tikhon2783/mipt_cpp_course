#include "agent_rules.h"

#include <memory>
#include <vector>

#include "combinators.h"
#include "conditions.h"
#include "make_rule.h"

namespace nano_edr {

std::unique_ptr<AnyOf> RuleCmdTemp() {
    auto rule = std::make_unique<AnyOf>();
    rule->Add(std::make_unique<FieldContains>("cmdline", "\\appdata\\local\\temp\\"));
    rule->Add(std::make_unique<FieldContains>("cmdline", "/appdata/local/temp/"));
    rule->Add(std::make_unique<FieldContains>("cmdline", "\\windows\\temp\\"));
    rule->Add(std::make_unique<FieldContains>("cmdline", "/windows/temp/"));
    rule->Add(std::make_unique<FieldContains>("cmdline", "%temp%"));
    rule->Add(std::make_unique<FieldContains>("cmdline", "%tmp%"));
    return rule;
}


std::unique_ptr<MatchRule> RuleScriptHostFromTemp() {
    auto rule = std::make_unique<MatchRule>("script_host_from_temp", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
    auto host = std::make_unique<AnyOf>();
    host->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    host->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
    rule->AddCondition(std::move(host));
    rule->AddCondition(RuleCmdTemp());
    rule->AddAction("kill_process");
    rule->AddAction("quarantine_source");
    rule->AddAction("delete_dropped");
    return rule;
}

std::unique_ptr<MatchRule> RuleLolbinDownload() {
    auto rule = std::make_unique<MatchRule>("lolbin_download", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
    auto lolbin_image = std::make_unique<AnyOf>();
    lolbin_image->Add(std::make_unique<FieldContains>("image", "certutil.exe"));
    lolbin_image->Add(std::make_unique<FieldContains>("image", "bitsadmin.exe"));
    rule->AddCondition(std::move(lolbin_image));
    auto lolbin_cmdline = std::make_unique<AnyOf>();
    lolbin_cmdline->Add(std::make_unique<FieldContains>("cmdline", "urlcache"));
    lolbin_cmdline->Add(std::make_unique<FieldContains>("cmdline", "transfer"));
    lolbin_cmdline->Add(std::make_unique<FieldContains>("cmdline", "http:"));
    lolbin_cmdline->Add(std::make_unique<FieldContains>("cmdline", "https:"));
    rule->AddCondition(std::move(lolbin_cmdline));
    rule->AddAction("kill_process");
    return rule;
}

std::unique_ptr<MatchRule> RuleHiddenPowershell() {
    auto rule = std::make_unique<MatchRule>("hidden_powershell", Severity::kMedium);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
    auto ps_image = std::make_unique<AnyOf>();
    ps_image->Add(std::make_unique<FieldContains>("image", "powershell.exe"));
    ps_image->Add(std::make_unique<FieldContains>("image", "pwsh.exe"));
    rule->AddCondition(std::move(ps_image));
    auto ps_cmdline = std::make_unique<AnyOf>();
    ps_cmdline->Add(std::make_unique<FieldContains>("cmdline", "-w hidden"));
    ps_cmdline->Add(std::make_unique<FieldContains>("cmdline", "-windowstyle hidden"));
    ps_cmdline->Add(std::make_unique<FieldContains>("cmdline", "-enc"));
    ps_cmdline->Add(std::make_unique<FieldContains>("cmdline", "-encodedcommand"));
    rule->AddCondition(std::move(ps_cmdline));
    rule->AddAction("kill_process");
    return rule;
}

std::unique_ptr<MatchRule> RuleAutostartWrite() {
    auto rule = std::make_unique<MatchRule>("autostart_write", Severity::kHigh);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite, EventType::kFileMove}));
    auto paths_autostart = std::make_unique<AnyOf>();
    paths_autostart->Add(std::make_unique<FieldContains>("path", "\\start menu\\programs\\startup\\"));
    paths_autostart->Add(std::make_unique<FieldContains>("path", "/start menu/programs/startup/"));
    paths_autostart->Add(std::make_unique<FieldContains>("to", "\\start menu\\programs\\startup\\"));
    paths_autostart->Add(std::make_unique<FieldContains>("to", "/start menu/programs/startup/"));
    rule->AddCondition(std::move(paths_autostart));
    rule->AddAction("delete_evidence");
    return rule;
}

std::unique_ptr<MatchRule> RuleRansomExtension() {
    auto rule = std::make_unique<MatchRule>("ransom_extension", Severity::kCritical);
    rule->AddCondition(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kFileCreate, EventType::kFileWrite, EventType::kFileMove}));
    auto paths_ransom = std::make_unique<AnyOf>();
    paths_ransom->Add(std::make_unique<FieldEndsWith>("path", ".locked"));
    paths_ransom->Add(std::make_unique<FieldEndsWith>("to", ".locked"));
    rule->AddCondition(std::move(paths_ransom));
    rule->AddAction("kill_process");
    rule->AddAction("quarantine_image");
    return rule;
}

std::unique_ptr<SequenceRule> RuleScriptHostBeacon() {
    auto rule = std::make_unique<SequenceRule>("script_host_beacon", Severity::kCritical, 10000);
    auto start = std::make_unique<AllOf>();
    start->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
    auto beacon_host = std::make_unique<AnyOf>();
    beacon_host->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    beacon_host->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
    start->Add(std::move(beacon_host));
    start->Add(RuleCmdTemp());
    rule->SetFirst(std::move(start));
    rule->SetSecond(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kNetConnect}));
    rule->AddAction("kill_process");
    return rule;
}

std::unique_ptr<ThresholdRule<128>> RuleMassWrite() {
    auto rule = MakeRule<ThresholdRule<128>>(
        "ransomware_mass_write", Severity::kCritical,
        20,      // сколько событий
        10000,   // за сколько миллисекунд
        "pid");  // порог считается для каждого pid отдельно
    rule->SetCondition(MakeCondition(AllOfStatic(
        EventTypeIs({EventType::kFileCreate,
                     EventType::kFileWrite,
                     EventType::kFileMove}),
        AnyOfStatic(FieldContains("path", "\\Documents\\"),
                    FieldContains("path", "\\Desktop\\"),
                    FieldContains("to", "\\Documents\\"),
                    FieldContains("to", "\\Desktop\\")))));
    rule->AddAction("kill_process");
    rule->AddAction("quarantine_image");
    return rule;
}

std::unique_ptr<SequenceRule> RuleShellSpawnsScriptHost(const EntityModel* model) {
    auto rule = MakeRule<SequenceRule>("shell_spawns_script_host",
                                       Severity::kHigh, 30000);
    auto shell_start = std::make_unique<AllOf>();
    shell_start->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
    auto shell_image = std::make_unique<AnyOf>();
    shell_image->Add(std::make_unique<FieldEndsWith>("image", "cmd.exe"));
    shell_image->Add(std::make_unique<FieldEndsWith>("image", "powershell.exe"));
    shell_image->Add(std::make_unique<FieldEndsWith>("image", "pwsh.exe"));
    shell_start->Add(std::move(shell_image));
    rule->SetFirst(std::move(shell_start));

    auto host_start = std::make_unique<AllOf>();
    host_start->Add(std::make_unique<EventTypeIs>(std::vector<EventType>{EventType::kProcessStart}));
    auto host_image = std::make_unique<AnyOf>();
    host_image->Add(std::make_unique<FieldEndsWith>("image", "wscript.exe"));
    host_image->Add(std::make_unique<FieldEndsWith>("image", "cscript.exe"));
    host_start->Add(std::move(host_image));
    rule->SetSecond(std::move(host_start));

    rule->set_child_of_prev(true);
    rule->SetModel(model);
    rule->AddAction("kill_process");
    return rule;
}

std::unique_ptr<LambdaRule> RuleSpawnedFromDroppedFile(const EntityModel* model) {
    auto rule = MakeRule<LambdaRule>(
        "spawned_from_dropped_file", Severity::kHigh,
        [model](const Event& event) {
            if (event.event_type() != EventType::kProcessStart) {
                return false;
            }
            const FileRecord* source = model->FindSpawnSource(event.pid());
            return source != nullptr && !source->writer_pid.empty() &&
                   source->writer_pid != event.pid();
        });
    rule->AddAction("kill_process");
    rule->AddAction("quarantine_source");
    rule->AddAction("delete_dropped");
    return rule;
}

RuleEngine BuildAgentRules(const EntityModel* model) {
    RuleEngine engine;

    auto rule_temp = RuleCmdTemp();
    engine.AddRule(RuleScriptHostFromTemp());
    engine.AddRule(RuleAutostartWrite());
    engine.AddRule(RuleHiddenPowershell());
    engine.AddRule(RuleLolbinDownload());
    engine.AddRule(RuleMassWrite());
    engine.AddRule(RuleScriptHostBeacon());
    engine.AddRule(RuleRansomExtension());
    engine.AddRule(RuleShellSpawnsScriptHost(model));
    engine.AddRule(RuleSpawnedFromDroppedFile(model));
    return engine;
}

}  // namespace nano_edr
