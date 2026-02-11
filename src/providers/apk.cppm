module;

#include <array>
#include <cstdio>
#include <memory>
#include <set>
#include <sstream>
#include <string>

export module paclook.providers.apk;

import paclook.provider;
import paclook.util;

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
    std::string output = exec_command("apk info 2>/dev/null");

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

export class ApkProvider : public Provider {
public:
    std::string name() const override { return "apk"; }
    bool is_available() const override;
    SearchResult search(const std::string& query) const override;
    std::string install_command(const Package& pkg) const override;

    std::string source_color(const std::string& source) const override {
        if (source == "community") return "\033[33m";  // yellow
        return "\033[34m";                             // blue for main
    }
};

bool ApkProvider::is_available() const {
    return command_exists("apk");
}

SearchResult ApkProvider::search(const std::string& query) const {
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

    // apk search -v returns: package-name-version - description
    std::string cmd = "apk search -v '" + escaped_query + "' 2>/dev/null";
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

        // Format: package-name-version - description
        size_t sep = line.find(" - ");
        if (sep == std::string::npos) continue;

        std::string name_version = line.substr(0, sep);
        std::string description = line.substr(sep + 3);

        Package pkg;

        // Extract name and version (version is after last hyphen followed by digit)
        // e.g., "firefox-esr-115.6.0-r0" -> name="firefox-esr", version="115.6.0-r0"
        size_t ver_start = std::string::npos;
        for (size_t i = name_version.size(); i > 0; --i) {
            if (name_version[i-1] == '-' && i < name_version.size() && isdigit(name_version[i])) {
                ver_start = i - 1;
                break;
            }
        }

        if (ver_start != std::string::npos) {
            pkg.name = name_version.substr(0, ver_start);
            pkg.version = name_version.substr(ver_start + 1);
        } else {
            pkg.name = name_version;
        }

        pkg.description = description;
        pkg.source = "alpine";
        pkg.installed = (installed.find(pkg.name) != installed.end());

        result.packages.push_back(pkg);
    }

    // Sort by relevance (exact match, starts with, contains, shorter names first)
    sort_by_relevance(result.packages, query);

    return result;
}

std::string ApkProvider::install_command(const Package& pkg) const {
    return "sudo apk add " + pkg.name;
}

} // namespace paclook
