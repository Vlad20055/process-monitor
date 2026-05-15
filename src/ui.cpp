#include "process_monitor/ui.hpp"

#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <poll.h>
#include <sstream>
#include <string>
#include <unistd.h>

namespace process_monitor {
namespace {

std::string state_description(char state) {
    switch (state) {
    case 'R': return "running";
    case 'S': return "sleeping";
    case 'D': return "disk sleep";
    case 'T': return "stopped";
    case 't': return "tracing stop";
    case 'Z': return "zombie";
    case 'X':
    case 'x': return "dead";
    case 'I': return "idle";
    default:  return "unknown";
    }
}

std::string truncate_text(const std::string& text, std::size_t width) {
    if (text.size() <= width) {
        return text;
    }
    if (width <= 3) {
        return text.substr(0, width);
    }
    return text.substr(0, width - 3) + "...";
}

double bytes_to_mib(std::uint64_t bytes) {
    return static_cast<double>(bytes) / 1024.0 / 1024.0;
}

}  // namespace

ConsoleUI::ConsoleUI(AppConfig config)
    : config_(config),
      watch_interval_seconds_(config.initial_watch_interval_seconds) {}

void ConsoleUI::push_status_message(const std::string& msg) {
    status_message_ = msg;
    recent_messages_.push_back(msg);
    if (recent_messages_.size() > kMaxRecentMessages) {
        recent_messages_.erase(recent_messages_.begin());
    }
}

std::string ConsoleUI::trim(const std::string& text) {
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }

    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return text.substr(begin, end - begin);
}

bool ConsoleUI::parse_int(const std::string& text, int& value) {
    try {
        std::size_t pos = 0;
        const long parsed = std::stol(text, &pos, 10);
        if (pos != text.size()) {
            return false;
        }
        if (parsed < std::numeric_limits<int>::min() || parsed > std::numeric_limits<int>::max()) {
            return false;
        }
        value = static_cast<int>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

void ConsoleUI::print_help() const {
    std::cout
        << "Commands:\n"
        << "  help                 show this help\n"
        << "  refresh | r          refresh process list\n"
        << "  watch <sec>          enable auto refresh every N seconds\n"
        << "  stop                 disable auto refresh\n"
        << "  kill <pid>           terminate process with SIGTERM\n"
        << "  suspend <pid>        stop process with SIGSTOP\n"
        << "  resume <pid>         continue process with SIGCONT\n"
        << "  nice <pid> <value>   change process priority (nice value)\n"
        << "  quit | exit          leave the program\n";
}

void ConsoleUI::refresh_and_print() {
    processes_ = reader_.list_processes();

    // Apply CLI filters from config_
    if (!config_.name_filter.empty() || config_.min_cpu_seconds > 0.0 || !config_.user_filter.empty()) {
        std::vector<ProcessInfo> filtered;
        filtered.reserve(processes_.size());

        const std::string name_filter_lower = [&]() {
            std::string s = config_.name_filter;
            for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }();

        for (const auto& p : processes_) {
            bool keep = true;

            if (!config_.name_filter.empty()) {
                std::string hay = p.name + " " + p.command_line;
                for (char& c : hay) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (hay.find(name_filter_lower) == std::string::npos) {
                    keep = false;
                }
            }

            if (keep && config_.min_cpu_seconds > 0.0) {
                if (p.cpu_time_seconds < config_.min_cpu_seconds) {
                    keep = false;
                }
            }

            if (keep && !config_.user_filter.empty()) {
                if (p.user != config_.user_filter) {
                    keep = false;
                }
            }

            if (keep) {
                filtered.push_back(p);
            }
        }

        processes_.swap(filtered);
    }

    // Always perform a full clear and move to home to ensure no previous
    // content remains on the screen before printing the refreshed list.
    // std::cout << "\033[2J\033[H";
    std::system("clear");
    first_update_ = false;

    std::cout << "Process Monitor\n";
    std::cout << "Processes: " << processes_.size();
    if (!config_.name_filter.empty() || config_.min_cpu_seconds > 0.0 || !config_.user_filter.empty()) {
        std::cout << " | filters:";
        if (!config_.name_filter.empty()) {
            std::cout << " name='" << config_.name_filter << "'";
        }
        if (config_.min_cpu_seconds > 0.0) {
            std::cout << " min_cpu>=" << config_.min_cpu_seconds << "s";
        }
        if (!config_.user_filter.empty()) {
            std::cout << " user='" << config_.user_filter << "'";
        }
    }

    if (watch_interval_seconds_ > 0) {
        std::cout << " | watch: " << watch_interval_seconds_ << "s";
    }

    std::cout << "\n";

    // Show recent operation messages
    if (!recent_messages_.empty()) {
        for (const auto& m : recent_messages_) {
            std::cout << m << "\n";
        }
        std::cout << "\n";
    } else if (!status_message_.empty()) {
        std::cout << status_message_ << "\n\n";
    } else {
        std::cout << "\n";
    }

    last_printed_lines_ = print_processes();
    std::cout.flush();
}

int ConsoleUI::print_processes() {
    std::cout << std::left
              << std::setw(7)  << "PID"
              << std::setw(12) << "USER"
              << std::setw(6)  << "NI"
              << std::setw(4)  << "S"
              << std::setw(14) << "CPU(s)"
              << std::setw(12) << "RSS(MiB)"
              << std::setw(12) << "VSZ(MiB)"
              << "CMD\n";

    std::cout << std::string(76, '-') << "\n";

    int lines_printed = 2;  // header + separator line

    if (processes_.empty()) {
        std::cout << "(no processes)\n";
        return lines_printed + 1;
    }

    for (const auto& p : processes_) {
        std::ostringstream cpu;
        cpu << std::fixed << std::setprecision(1) << p.cpu_time_seconds;

        std::ostringstream rss;
        rss << std::fixed << std::setprecision(1) << bytes_to_mib(p.resident_memory_bytes);

        std::ostringstream vms;
        vms << std::fixed << std::setprecision(1) << bytes_to_mib(p.virtual_memory_bytes);

        std::cout << std::left
                  << std::setw(7)  << p.pid
                  << std::setw(12) << truncate_text(p.user, 11)
                  << std::setw(6)  << p.nice
                  << std::setw(4)  << p.state
                  << std::setw(14) << cpu.str()
                  << std::setw(12) << rss.str()
                  << std::setw(12) << vms.str()
                  << truncate_text(p.command_line, 80)
                  << "\n";
        ++lines_printed;
    }

    return lines_printed;
}

bool ConsoleUI::handle_command(const std::string& line) {
    const std::string cmdline = trim(line);
    if (cmdline.empty()) {
        return true;
    }

    std::istringstream iss(cmdline);
    std::string command;
    iss >> command;

    if (command == "help") {
        print_help();
        return true;
    }

    if (command == "refresh" || command == "r") {
        refresh_and_print();
        return true;
    }

    if (command == "watch") {
        int seconds = 0;
        if (!(iss >> seconds) || seconds <= 0) {
            push_status_message("Invalid interval.");
            return true;
        }

        watch_interval_seconds_ = seconds;
        push_status_message(std::string("Auto refresh enabled: every ") + std::to_string(watch_interval_seconds_) + " second(s).");
        refresh_and_print();
        return true;
    }

    if (command == "stop") {
        watch_interval_seconds_ = 0;
        push_status_message("Auto refresh disabled.");
        refresh_and_print();
        return true;
    }

    if (command == "kill" || command == "suspend" || command == "resume") {
        int pid = 0;
        if (!(iss >> pid) || pid <= 0) {
            push_status_message("Invalid PID.");
            return true;
        }

        std::string error;
        bool ok = false;

        if (command == "kill") {
            ok = manager_.terminate_process(pid, error);
        } else if (command == "suspend") {
            ok = manager_.suspend_process(pid, error);
        } else {
            ok = manager_.resume_process(pid, error);
        }


        if (!ok) {
            if (command == "kill") {
                push_status_message(std::string("Terminate ") + std::to_string(pid) + " failed: " + error);
            } else if (command == "suspend") {
                push_status_message(std::string("Suspend ") + std::to_string(pid) + " failed: " + error);
            } else if (command == "resume") {
                push_status_message(std::string("Resume ") + std::to_string(pid) + " failed: " + error);
            } else {
                push_status_message(std::string("Operation failed: ") + error);
            }
        } else {
            if (command == "kill") {
                push_status_message(std::string("Process ") + std::to_string(pid) + " terminated.");
            } else if (command == "suspend") {
                push_status_message(std::string("Process ") + std::to_string(pid) + " suspended.");
            } else if (command == "resume") {
                push_status_message(std::string("Process ") + std::to_string(pid) + " resumed.");
            } else {
                push_status_message("Operation completed.");
            }
        }

        refresh_and_print();
        return true;
    }

    if (command == "nice") {
        int pid = 0;
        int value = 0;

        if (!(iss >> pid >> value) || pid <= 0 || value < -20 || value > 19) {
            push_status_message("Usage: nice <pid> <value>, where value is in range [-20; 19].");
            return true;
        }

        std::string error;
        if (!manager_.set_priority(pid, value, error)) {
            push_status_message(std::string("Operation failed: ") + error);
        } else {
            push_status_message("Priority changed.");
        }

        refresh_and_print();
        return true;
    }

    if (command == "quit" || command == "exit") {
        running_ = false;
        return false;
    }

    status_message_ = "Unknown command. Type 'help'.";
    return true;
}

int ConsoleUI::run() {
    if (config_.one_shot) {
        refresh_and_print();
        return 0;
    }

    if (watch_interval_seconds_ > 0) {
        std::cout << "Auto refresh enabled on startup: every "
                  << watch_interval_seconds_ << " second(s).\n";
    }

    print_help();
    refresh_and_print();

    while (running_) {
        if (watch_interval_seconds_ > 0) {
            pollfd pfd{};
            pfd.fd = STDIN_FILENO;
            pfd.events = POLLIN;

            const int timeout_ms = watch_interval_seconds_ * 1000;
            const int rc = ::poll(&pfd, 1, timeout_ms);

            if (rc < 0) {
                if (errno == EINTR) {
                    continue;
                }
                std::cerr << "poll() failed.\n";
                break;
            }

            if (rc == 0) {
                refresh_and_print();
                continue;
            }

            std::string line;
            if (!std::getline(std::cin, line)) {
                break;
            }

            handle_command(line);
            continue;
        }

        std::cout << "pm> " << std::flush;

        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (!handle_command(line)) {
            break;
        }
    }

    return 0;
}

}  // namespace process_monitor