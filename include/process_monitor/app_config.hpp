#pragma once

namespace process_monitor {

struct AppConfig {
    int initial_watch_interval_seconds = 0;
    bool one_shot = false;
};

}  // namespace process_monitor