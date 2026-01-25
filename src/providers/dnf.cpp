#include "providers/dnf.hpp"
#include "util.hpp"
#include <array>
#include <cstdio>
#include <memory>
#include <sstream>
#include <set>

namespace paclook {

namespace {

std::string exec_command(const std::string& cmd) {
    std::array<char, 4096> buffer;
    std::string result;

    auto pipe_deleter = [](FILE* f) { if (f) pclose(f); };
    std::unique_ptr<FILE, decltype(pipe_deleter)> pipe(
        popen(cmd.c_str(), "r"), pipe_deleter);

    if (!pipe) {
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}

bool command_exists(const std::string& cmd) {
    std::string check = "command -v " + cmd + " > /dev/null 2>&1";
    return system(check.c_str()) == 0;
}

std::set<std::string> get_installed_packages() {
    std::set<std::string> installed;
    std::string output = exec_command("rpm -qa --qf '%{NAME}\\n' 2>/dev/null");

    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            installed.insert(line);
        }
    }
    return installed;
}

} // anonymous namespace

bool DnfProvider::is_available() const {
    return command_exists("dnf");
}

SearchResult DnfProvider::search(const std::string& query) const {
    SearchResult result;

    if (query.empty()) {
        return result;
    }

    // Escape query for shell
    std::string escaped_query = query;
    for (size_t i = 0; i < escaped_query.size(); ++i) {
        if (escaped_query[i] == '\'' || escaped_query[i] == '"' ||
            escaped_query[i] == '\\' || escaped_query[i] == '`' ||
            escaped_query[i] == '$') {
            escaped_query.insert(i, "\\");
            ++i;
        }
    }

    // dnf search output: name.arch : description
    // Use --quiet to reduce noise
    std::string cmd = "dnf search --quiet '" + escaped_query + "' 2>/dev/null";
    std::string output = exec_command(cmd);

    if (output.empty()) {
        return result;
    }

    // Get installed packages for status
    auto installed = get_installed_packages();

    std::istringstream stream(output);
    std::string line;
    bool in_results = false;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        // Skip header lines like "=== Name Matched: xxx ==="
        if (line.find("===") != std::string::npos) {
            in_results = true;
            continue;
        }

        if (!in_results && line.find(" : ") == std::string::npos) {
            continue;
        }

        // Format: name.arch : description
        size_t sep = line.find(" : ");
        if (sep == std::string::npos) continue;

        std::string name_arch = line.substr(0, sep);
        std::string description = line.substr(sep + 3);

        // Extract name (remove .arch suffix)
        Package pkg;
        size_t dot = name_arch.rfind('.');
        if (dot != std::string::npos) {
            pkg.name = name_arch.substr(0, dot);
        } else {
            pkg.name = name_arch;
        }

        pkg.description = description;
        pkg.source = "fedora";
        pkg.installed = (installed.find(pkg.name) != installed.end());

        result.packages.push_back(pkg);
    }

    // Sort by relevance (exact match, starts with, contains, shorter names first)
    sort_by_relevance(result.packages, query);

    return result;
}

std::string DnfProvider::install_command(const Package& pkg) const {
    return "sudo dnf install " + pkg.name;
}

} // namespace paclook
