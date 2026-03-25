#pragma once

#include <cstdint>
#include <string>

namespace process_monitor {

struct ProcessInfo {
    int pid = 0;
    int ppid = 0;
    int uid = 0;

    char state = '?';
    int priority = 0;
    int nice = 0;

    std::uint64_t virtual_memory_bytes = 0;
    std::uint64_t resident_memory_bytes = 0;
    double cpu_time_seconds = 0.0;

    std::string user;
    std::string name;
    std::string command_line;
};

}  // namespace process_monitor