#include "response.h"

#include <cctype>
#include <memory>
#include <print>
#include <string>
#include <utility>
#include <vector>

#include "detection.h"
#include "entity_model.h"
#include "event.h"
#include "fields.h"
#include "os_handle.h"

namespace nano_edr {

namespace {

bool StartsWithIgnoreCase(const std::string& path, const std::string& prefix) {
    if (path.size() < prefix.size()) {
        return false;
    }
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(path[i]);
        if (static_cast<char>(std::tolower(c)) != prefix[i]) {
            return false;
        }
    }
    return true;
}

bool IsSystemPath(const std::string& path) {
    return StartsWithIgnoreCase(path, "c:\\windows\\") ||
           StartsWithIgnoreCase(path, "c:\\program files");
}

}  // namespace

// --- ResponsePlanner -------------------------------------------------------

ResponsePlanner::ResponsePlanner(const EntityModel* model,
                                 std::string quarantine_dir)
    : model_(model), quarantine_dir_(quarantine_dir) {}

ResponsePlan ResponsePlanner::Plan(const Detection& detection) const {
    ResponsePlan plan;
    // процесс детекта и все его потомки; процесс детекта стоит первым
    const std::vector<std::string> tree =
        model_->GetProcessTree(detection.pid);

    auto add_quarantine = [&](const std::string& path,
                              const std::string& why) {
        if (IsSystemPath(path)) {
            return;
        }
        const std::string to = QuarantinePath(path, detection.pid);
        if (to.empty()) {
            return;
        }
        Action action;
        action.kind = ActionKind::kQuarantineFile;
        action.path = path;
        action.to = to;
        action.reason = detection.rule + why;
        plan.Add(std::move(action));
    };

    auto add_delete = [&](const std::string& path, const std::string& why) {
        if (IsSystemPath(path)) {
            return;
        }
        Action action;
        action.kind = ActionKind::kDeleteFile;
        action.path = path;
        action.reason = detection.rule + why;
        plan.Add(std::move(action));
    };

    for (const std::string& name : detection.actions) {
        if (name == "kill_process") {
            for (std::size_t i = tree.size(); i > 0; --i) {
                const std::string& pid = tree[i - 1];
                Action action;
                action.kind = ActionKind::kKillProcess;
                action.pid = pid;
                // запись есть у каждого процесса из tree;
                // start — ноль, если запуска процесса модель не видела
                action.expected_start = model_->FindProcess(pid)->start;
                action.reason = detection.rule;
                if (pid != detection.pid) {
                    action.reason += ", потомок " + detection.pid;
                }
                plan.Add(std::move(action));
            }
        } else if (name == "quarantine_source") {
            const FileRecord* source = model_->FindSpawnSource(detection.pid);
            if (source != nullptr) {
                add_quarantine(source->path, ", источник запуска");
            }
        } else if (name == "quarantine_image") {
            const ProcessRecord* process = model_->FindProcess(detection.pid);
            if (process != nullptr) {
                add_quarantine(process->image, ", образ процесса");
            }
        } else if (name == "delete_dropped") {
            for (const std::string& pid : tree) {
                const std::vector<std::string> files =
                    model_->GetFilesCreatedBy(pid, true);
                for (const std::string& path : files) {
                    add_delete(path, ", принесён " + pid);
                }
            }
        } else if (name == "delete_evidence") {
            for (const std::unique_ptr<Event>& event : detection.evidence) {
                if (event == nullptr) {
                    continue;
                }
                const std::string* path =
                    FindField(*event, event->event_type() == EventType::kFileMove
                                          ? "to"
                                          : "path");
                if (path != nullptr) {
                    add_delete(*path, ", файл из детекта");
                }
            }
        }
        // незнакомое имя пропускается
    }
    return plan;
}

std::string ResponsePlanner::QuarantinePath(const std::string& path,
                                            const std::string& pid) const {
    if (quarantine_dir_.empty()) {
        return "";
    }
    const std::size_t slash = path.find_last_of('\\');
    const std::string file_name =
        slash == std::string::npos ? path : path.substr(slash + 1);
    return quarantine_dir_ + "\\" + pid + "-" + file_name;
}

// --- Responder -------------------------------------------------------------

Responder::Responder(OsHandle* os, bool dry_run) : os_(os), dry_run_(dry_run) {}

void Responder::Execute(const ResponsePlan& plan) {
    for (const Action& action : plan.actions()) {
        ++stats_.planned;

        // цель, по которой уже отработали: на одном процессе срабатывает
        // несколько правил
        bool repeat = false;
        for (const std::string& target : done_) {
            if (target == action.target()) {
                repeat = true;
                break;
            }
        }
        if (repeat) {
            ++stats_.skipped_repeat;
            continue;
        }
        done_.push_back(action.target());

        if (dry_run_ || os_ == nullptr) {
            std::print("[PLAN] {}\n", ToString(action));
            continue;
        }

        switch (action.kind) {
            case ActionKind::kKillProcess:
                Kill(action);
                break;
            case ActionKind::kQuarantineFile:
                MoveToQuarantine(action);
                break;
            case ActionKind::kDeleteFile:
                Delete(action);
                break;
        }
    }
}

}  // namespace nano_edr
