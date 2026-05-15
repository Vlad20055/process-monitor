#include "process_monitor/app_config.hpp"
#include "process_monitor/ui.hpp"

#include <iostream>
#include <string>

namespace {

void print_usage() {
    std::cout
    << "Usage: process_monitor [--once] [--watch N] [--name-filter STR] [--min-cpu SECS] [--user NAME]\n"
        << "\n"
        << "  --once      print process list once and exit\n"
        << "  --watch N   start in auto-refresh mode with interval N seconds\n"
    << "  --name-filter STR   only show processes whose name or cmdline contains STR\n"
    << "  --min-cpu SECS      only show processes with CPU time >= SECS\n"
    << "  --user NAME         only show processes owned by user NAME\n"
        << "  --help      show this help\n";
}

bool parse_positive_int(const std::string& text, int& value) {
    try {
        std::size_t pos = 0;
        const long parsed = std::stol(text, &pos, 10);
        if (pos != text.size() || parsed <= 0 || parsed > 3600) {
            return false;
        }
        value = static_cast<int>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_nonneg_double(const std::string& text, double& value) {
    try {
        std::size_t pos = 0;
        const double parsed = std::stod(text, &pos);
        if (pos != text.size() || parsed < 0.0) {
            return false;
        }
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        process_monitor::AppConfig config;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];

            if (arg == "--help" || arg == "-h") {
                print_usage();
                return 0;
            }

            if (arg == "--once") {
                config.one_shot = true;
                continue;
            }

            if (arg == "--watch") {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value for --watch.\n";
                    print_usage();
                    return 1;
                }

                int seconds = 0;
                if (!parse_positive_int(argv[++i], seconds)) {
                    std::cerr << "Invalid value for --watch.\n";
                    return 1;
                }

                config.initial_watch_interval_seconds = seconds;
                continue;
            }

            if (arg == "--name-filter") {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value for --name-filter.\n";
                    print_usage();
                    return 1;
                }

                config.name_filter = argv[++i];
                continue;
            }

            if (arg == "--min-cpu") {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value for --min-cpu.\n";
                    print_usage();
                    return 1;
                }

                double val = 0.0;
                if (!parse_nonneg_double(argv[++i], val)) {
                    std::cerr << "Invalid value for --min-cpu.\n";
                    return 1;
                }

                config.min_cpu_seconds = val;
                continue;
            }

            if (arg == "--user") {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value for --user.\n";
                    print_usage();
                    return 1;
                }

                config.user_filter = argv[++i];
                continue;
            }

            std::cerr << "Unknown argument: " << arg << '\n';
            print_usage();
            return 1;
        }

        if (config.one_shot) {
            config.initial_watch_interval_seconds = 0;
        }

        process_monitor::ConsoleUI ui(config);
        return ui.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return 1;
    }
}