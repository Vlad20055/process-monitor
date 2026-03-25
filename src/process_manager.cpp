#include "process_monitor/process_manager.hpp"

#include <cerrno>
#include <cstring>
#include <signal.h>
#include <sys/resource.h>
#include <unistd.h>

namespace process_monitor {

bool ProcessManager::terminate_process(int pid, std::string& error) const {
    if (::kill(pid, SIGTERM) != 0) {
        error = std::strerror(errno);
        return false;
    }
    return true;
}

bool ProcessManager::suspend_process(int pid, std::string& error) const {
    if (::kill(pid, SIGSTOP) != 0) {
        error = std::strerror(errno);
        return false;
    }
    return true;
}

bool ProcessManager::resume_process(int pid, std::string& error) const {
    if (::kill(pid, SIGCONT) != 0) {
        error = std::strerror(errno);
        return false;
    }
    return true;
}

bool ProcessManager::set_priority(int pid, int nice_value, std::string& error) const {
    if (::setpriority(PRIO_PROCESS, static_cast<id_t>(pid), nice_value) != 0) {
        error = std::strerror(errno);
        return false;
    }
    return true;
}

}  // namespace process_monitor