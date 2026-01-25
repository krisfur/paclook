#include "providers/xbps.hpp"
#include <array>
#include <cstdio>
#include <memory>
#include <sstream>

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

} // anonymous namespace

bool XbpsProvider::is_available() const {
    return command_exists("xbps-query");
}

SearchResult XbpsProvider::search(const std::string& query) const {
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

    // xbps-query -Rs searches remote repos
    std::string cmd = "xbps-query -Rs '" + escaped_query + "' 2>/dev/null";
    std::string output = exec_command(cmd);

    if (output.empty()) {
        return result;
    }

    // Parse xbps-query output format:
    // [-] package-name-version    Description text here
    // [*] installed-pkg-version   Description for installed
    //
    // Where [-] = not installed, [*] = installed
    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.size() < 5) continue;

        Package pkg;

        // Check installed status
        if (line.substr(0, 3) == "[*]") {
            pkg.installed = true;
        } else if (line.substr(0, 3) == "[-]") {
            pkg.installed = false;
        } else {
            continue;  // Unknown format
        }

        // Skip the "[*] " or "[-] " prefix
        size_t pos = 4;

        // Find the package name-version (ends at multiple spaces before description)
        size_t desc_start = line.find("  ", pos);
        if (desc_start == std::string::npos) {
            // No description, just package name
            std::string name_version = line.substr(pos);
            // Remove trailing whitespace
            while (!name_version.empty() && isspace(name_version.back())) {
                name_version.pop_back();
            }

            // Split name and version at last hyphen
            size_t last_hyphen = name_version.rfind('-');
            if (last_hyphen != std::string::npos && last_hyphen > 0) {
                pkg.name = name_version.substr(0, last_hyphen);
                pkg.version = name_version.substr(last_hyphen + 1);
            } else {
                pkg.name = name_version;
            }
        } else {
            std::string name_version = line.substr(pos, desc_start - pos);

            // Split name and version at last hyphen
            size_t last_hyphen = name_version.rfind('-');
            if (last_hyphen != std::string::npos && last_hyphen > 0) {
                pkg.name = name_version.substr(0, last_hyphen);
                pkg.version = name_version.substr(last_hyphen + 1);
            } else {
                pkg.name = name_version;
            }

            // Get description (skip leading spaces)
            size_t desc_text = line.find_first_not_of(' ', desc_start);
            if (desc_text != std::string::npos) {
                pkg.description = line.substr(desc_text);
            }
        }

        pkg.source = "void";  // Void Linux repo
        result.packages.push_back(pkg);
    }

    return result;
}

std::string XbpsProvider::install_command(const Package& pkg) const {
    return "sudo xbps-install " + pkg.name;
}

} // namespace paclook
