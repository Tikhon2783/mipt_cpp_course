#ifndef NANO_EDR_SOURCES_H
#define NANO_EDR_SOURCES_H

#include <fstream>

#include "agent.h"
#include "os_handle.h"

namespace nano_edr {

class FileSource {
 public:
    explicit FileSource(const std::string& path);
    void Run(Agent* agent);  // строка → EventParts → Event → HandleEvent

 private:
    std::ifstream log_;
};

class OsSource {
 public:
    explicit OsSource(const std::string& config_path);  // создаёт OsHandle
    void Run(Agent* agent);                             // Subscribe, Start, Wait в цикле

 private:
    OsHandle handle_;
};

}  // namespace nano_edr

#endif  // NANO_EDR_SOURCES_H
