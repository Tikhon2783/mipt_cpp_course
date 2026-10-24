#include "agent_rules.h"

#include "fields.h"

namespace nano_edr {
    
std::vector<MatchRule> BuildAgentRules() {
    std::vector<MatchRule> rules;

    // CheckScriptHostFromTemp
    MatchRule script_host("script_host_from_temp", Severity::kHigh);
    script_host.AddCondition(Condition::Equals("type", {"process_start"}));
    script_host.AddCondition(
        Condition::EndsWith("image", {"wscript.exe", "cscript.exe"}));
    script_host.AddCondition(Condition::Contains("cmdline", {
        "\\appdata\\local\\temp\\", "/appdata/local/temp/",
        "\\windows\\temp\\", "/windows/temp/",
        "%temp%", "%tmp%"}));
    script_host.AddAction("kill_process");
    rules.push_back(script_host);

    // CheckLolbinDownload
    MatchRule lolbin("lolbin_download", Severity::kHigh);
    lolbin.AddCondition(Condition::Equals("type", {"process_start"}));
    lolbin.AddCondition(
        Condition::Contains("image", {"certutil.exe", "bitsadmin.exe"}));
    lolbin.AddCondition(Condition::Contains("cmdline", {
        "urlcache", "transfer", "http:", "https:"}));
    lolbin.AddAction("kill_process");
    rules.push_back(lolbin);

    // CheckHiddenPowershell
    MatchRule powershell("hidden_powershell", Severity::kMedium);
    powershell.AddCondition(Condition::Equals("type", {"process_start"}));
    powershell.AddCondition(
        Condition::Contains("image", {"powershell.exe", "pwsh.exe"}));
    powershell.AddCondition(Condition::Contains("cmdline", {
        "-w hidden", "-windowstyle hidden", "-enc", "-encodedcommand"}));
    powershell.AddAction("kill_process");
    rules.push_back(powershell);

    // CheckAutostartWrite
    MatchRule autostart_by_path("autostart_write", Severity::kHigh);
    autostart_by_path.AddCondition(
        Condition::Equals("type", {"file_create", "file_write"}));
    autostart_by_path.AddCondition(Condition::Contains("path", {
        "\\start menu\\programs\\startup\\", "/start menu/programs/startup/"}));
    autostart_by_path.AddAction("quarantine_file");
    rules.push_back(autostart_by_path);

    MatchRule autostart_by_to("autostart_write", Severity::kHigh);
    autostart_by_to.AddCondition(Condition::Equals("type", {"file_move"}));
    autostart_by_to.AddCondition(Condition::Contains("to", {
        "\\start menu\\programs\\startup\\", "/start menu/programs/startup/"}));
    autostart_by_to.AddAction("quarantine_file");
    rules.push_back(autostart_by_to);

    // CheckRansomExtension
    MatchRule ransom_by_path("ransom_extension", Severity::kCritical);
    ransom_by_path.AddCondition(
        Condition::Equals("type", {"file_create", "file_write"}));
    ransom_by_path.AddCondition(Condition::EndsWith("path", {".locked"}));
    ransom_by_path.AddAction("quarantine_file");
    rules.push_back(ransom_by_path);

    MatchRule ransom_by_to("ransom_extension", Severity::kCritical);
    ransom_by_to.AddCondition(Condition::Equals("type", {"file_move"}));
    ransom_by_to.AddCondition(Condition::EndsWith("to", {".locked"}));
    ransom_by_to.AddAction("quarantine_file");
    rules.push_back(ransom_by_to);

    return rules;
}

}  // namespace nano_edr
