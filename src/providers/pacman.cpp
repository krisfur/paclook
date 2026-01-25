#include "providers/pacman.hpp"
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

bool PacmanProvider::is_available() const {
    return command_exists("pacman");
}

SearchResult PacmanProvider::search(const std::string& query) const {
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

    std::string cmd = "pacman -Ss '" + escaped_query + "' 2>/dev/null";
    std::string output = exec_command(cmd);

    if (output.empty()) {
        return result;
    }

    // Parse pacman output format (same as paru for official repos):
    // repo/package-name version [installed]
    //     description
    std::istringstream stream(output);
    std::string line;
    Package current;
    bool has_package = false;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        // Check if this is a package header line (starts with repo/)
        if (!line.empty() && line[0] != ' ' && line[0] != '\t') {
            // Save previous package if exists
            if (has_package) {
                result.packages.push_back(current);
                current = Package{};
            }

            // Parse: repo/name version [installed]
            size_t slash_pos = line.find('/');
            if (slash_pos == std::string::npos) continue;

            current.source = line.substr(0, slash_pos);

            size_t name_start = slash_pos + 1;
            size_t space_pos = line.find(' ', name_start);
            if (space_pos == std::string::npos) continue;

            current.name = line.substr(name_start, space_pos - name_start);

            // Find version (after space, before next space or [installed])
            size_t version_start = space_pos + 1;
            size_t version_end = line.find(' ', version_start);
            if (version_end == std::string::npos) {
                current.version = line.substr(version_start);
            } else {
                current.version = line.substr(version_start, version_end - version_start);
            }

            // Check if installed
            current.installed = (line.find("[installed]") != std::string::npos ||
                                line.find("[Installed]") != std::string::npos);

            has_package = true;
        } else if (has_package) {
            // This is a description line (indented)
            size_t desc_start = line.find_first_not_of(" \t");
            if (desc_start != std::string::npos) {
                current.description = line.substr(desc_start);
            }
        }
    }

    // Don't forget the last package
    if (has_package) {
        result.packages.push_back(current);
    }

    return result;
}

std::string PacmanProvider::install_command(const Package& pkg) const {
    return "sudo pacman -S " + pkg.name;
}

} // namespace paclook
