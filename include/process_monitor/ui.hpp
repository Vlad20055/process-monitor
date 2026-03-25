#pragma once

#include "process_monitor/app_config.hpp"
#include "process_monitor/process_manager.hpp"
#include "process_monitor/process_reader.hpp"

#include <string>
#include <vector>

namespace process_monitor {

class ConsoleUI {
public:
    explicit ConsoleUI(AppConfig config = {});
    int run();

private:
    void refresh_and_print();
    void print_help() const;
    int print_processes();
    bool handle_command(const std::string& line);
    static std::string trim(const std::string& text);
    static bool parse_int(const std::string& text, int& value);

    AppConfig config_;
    ProcessReader reader_;
    ProcessManager manager_;

    bool running_ = true;
    int watch_interval_seconds_ = 0;
    std::vector<ProcessInfo> processes_;
    
    bool first_update_ = true;
    int last_printed_lines_ = 0;
};

}  // namespace process_monitor