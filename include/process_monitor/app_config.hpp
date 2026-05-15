#pragma once

#include <string>

namespace process_monitor {

struct AppConfig {
    int initial_watch_interval_seconds = 0;
    bool one_shot = false;
    std::string name_filter;
    double min_cpu_seconds = 0.0;
    std::string user_filter;
};

}  // namespace process_monitor