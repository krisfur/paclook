#include "providers/apt.hpp"
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
    std::string output = exec_command("dpkg-query -W -f='${Package}\\n' 2>/dev/null");

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

bool AptProvider::is_available() const {
    return command_exists("apt-cache");
}

SearchResult AptProvider::search(const std::string& query) const {
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

    // apt-cache search returns: package-name - description
    std::string cmd = "apt-cache search '" + escaped_query + "' 2>/dev/null";
    std::string output = exec_command(cmd);

    if (output.empty()) {
        return result;
    }

    // Get installed packages for status
    auto installed = get_installed_packages();

    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        // Format: package-name - description
        size_t sep = line.find(" - ");
        if (sep == std::string::npos) continue;

        Package pkg;
        pkg.name = line.substr(0, sep);
        pkg.description = line.substr(sep + 3);
        pkg.source = "apt";
        pkg.installed = (installed.find(pkg.name) != installed.end());

        result.packages.push_back(pkg);
    }

    // Sort by relevance (exact match, starts with, contains, shorter names first)
    sort_by_relevance(result.packages, query);

    return result;
}

std::string AptProvider::install_command(const Package& pkg) const {
    return "sudo apt install " + pkg.name;
}

} // namespace paclook
