#pragma once

#include <string>

namespace process_monitor {

class ProcessManager {
public:
    bool terminate_process(int pid, std::string& error) const;
    bool suspend_process(int pid, std::string& error) const;
    bool resume_process(int pid, std::string& error) const;
    bool set_priority(int pid, int nice_value, std::string& error) const;
};

}  // namespace process_monitor