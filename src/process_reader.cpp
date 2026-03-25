#include "process_monitor/process_reader.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <pwd.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unistd.h>

namespace process_monitor {
namespace fs = std::filesystem;

namespace {

std::string read_file(const fs::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("cannot open file: " + path.string());
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string trim_right_newlines(std::string text) {
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == '\0')) {
        text.pop_back();
    }
    return text;
}

std::string join_cmdline(std::string raw) {
    for (char& ch : raw) {
        if (ch == '\0') {
            ch = ' ';
        }
    }
    raw = trim_right_newlines(std::move(raw));

    while (!raw.empty() && raw.back() == ' ') {
        raw.pop_back();
    }
    return raw;
}

std::string user_name_from_uid(uid_t uid) {
    passwd pwd{};
    passwd* result = nullptr;

    long buf_size = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (buf_size < 0) {
        buf_size = 16384;
    }

    std::string buffer(static_cast<std::size_t>(buf_size), '\0');
    const int rc = getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &result);
    if (rc == 0 && result != nullptr && result->pw_name != nullptr) {
        return std::string(result->pw_name);
    }

    return std::to_string(static_cast<unsigned long>(uid));
}

bool is_pid_directory(const fs::directory_entry& entry, int& pid) {
    if (!entry.is_directory()) {
        return false;
    }

    const std::string name = entry.path().filename().string();
    if (name.empty()) {
        return false;
    }

    for (char ch : name) {
        if (ch < '0' || ch > '9') {
            return false;
        }
    }

    try {
        const long value = std::stol(name);
        if (value <= 0 || value > static_cast<long>(std::numeric_limits<int>::max())) {
            return false;
        }
        pid = static_cast<int>(value);
        return true;
    } catch (...) {
        return false;
    }
}

uid_t parse_uid_from_status(const fs::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return static_cast<uid_t>(0);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("Uid:", 0) == 0) {
            std::istringstream iss(line.substr(4));
            unsigned long uid = 0;
            iss >> uid;
            return static_cast<uid_t>(uid);
        }
    }

    return static_cast<uid_t>(0);
}

}  // namespace

ProcessInfo ProcessReader::read_process(int pid) const {
    const fs::path base = fs::path("/proc") / std::to_string(pid);

    ProcessInfo info;
    info.pid = pid;

    const std::string stat_content = read_file(base / "stat");
    const std::size_t lparen = stat_content.find('(');
    const std::size_t rparen = stat_content.rfind(')');

    if (lparen == std::string::npos || rparen == std::string::npos || rparen <= lparen) {
        throw std::runtime_error("invalid /proc stat format for pid " + std::to_string(pid));
    }

    info.name = stat_content.substr(lparen + 1, rparen - lparen - 1);

    const std::string after_comm = stat_content.substr(rparen + 2);
    std::istringstream iss(after_comm);

    std::string state_str;
    long long ppid = 0;
    long long priority = 0;
    long long nice = 0;
    unsigned long long vsize = 0;
    long long rss = 0;
    unsigned long long utime = 0;
    unsigned long long stime = 0;

    long long dummy_ll = 0;
    unsigned long long dummy_ull = 0;

    iss >> state_str
        >> ppid
        >> dummy_ll >> dummy_ll >> dummy_ll >> dummy_ll >> dummy_ll
        >> dummy_ll >> dummy_ll >> dummy_ll >> dummy_ll
        >> utime >> stime
        >> dummy_ll >> dummy_ll
        >> priority >> nice
        >> dummy_ll
        >> dummy_ll
        >> dummy_ull
        >> vsize >> rss;

    info.state = state_str.empty() ? '?' : state_str[0];
    info.ppid = static_cast<int>(ppid);
    info.priority = static_cast<int>(priority);
    info.nice = static_cast<int>(nice);
    info.virtual_memory_bytes = static_cast<std::uint64_t>(vsize);

    const long page_size = sysconf(_SC_PAGESIZE);
    if (page_size > 0 && rss > 0) {
        info.resident_memory_bytes = static_cast<std::uint64_t>(rss) * static_cast<std::uint64_t>(page_size);
    }

    const long ticks_per_second = sysconf(_SC_CLK_TCK);
    if (ticks_per_second > 0) {
        const unsigned long long total_ticks = utime + stime;
        info.cpu_time_seconds = static_cast<double>(total_ticks) / static_cast<double>(ticks_per_second);
    }

    info.uid = static_cast<int>(parse_uid_from_status(base / "status"));
    info.user = user_name_from_uid(static_cast<uid_t>(info.uid));

    std::string cmdline;
    try {
        cmdline = read_file(base / "cmdline");
    } catch (...) {
        cmdline.clear();
    }

    cmdline = join_cmdline(std::move(cmdline));
    info.command_line = cmdline.empty() ? info.name : cmdline;

    return info;
}

std::vector<ProcessInfo> ProcessReader::list_processes() const {
    std::vector<ProcessInfo> processes;
    processes.reserve(256);

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator("/proc", ec)) {
        if (ec) {
            break;
        }

        int pid = 0;
        if (!is_pid_directory(entry, pid)) {
            continue;
        }

        try {
            processes.push_back(read_process(pid));
        } catch (...) {
        }
    }

    std::sort(processes.begin(), processes.end(),
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.pid < b.pid;
              });

    return processes;
}

}  // namespace process_monitor