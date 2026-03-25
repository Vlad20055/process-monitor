#pragma once

#include "process_monitor/process.hpp"

#include <limits>
#include <vector>

namespace process_monitor {

class ProcessReader {
public:
    std::vector<ProcessInfo> list_processes() const;
    ProcessInfo read_process(int pid) const;
};

}  // namespace process_monitor