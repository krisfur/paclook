module;

#include <array>
#include <cstdio>
#include <memory>
#include <set>
#include <sstream>
#include <string>

export module paclook.providers.brew;

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

    // Get installed formulae
    std::string output = exec_command("brew list --formula 2>/dev/null");
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            installed.insert(line);
        }
    }

    // Get installed casks
    output = exec_command("brew list --cask 2>/dev/null");
    std::istringstream stream2(output);
    while (std::getline(stream2, line)) {
        if (!line.empty()) {
            installed.insert(line);
        }
    }

    return installed;
}

} // anonymous namespace

export class BrewProvider : public Provider {
public:
    std::string name() const override { return "brew"; }
    bool is_available() const override;
    SearchResult search(const std::string& query) const override;
    std::string install_command(const Package& pkg) const override;

    std::string source_color(const std::string& source) const override {
        if (source == "cask") return "\033[35m";  // magenta for casks
        return "\033[33m";                        // yellow for formulae
    }
};

bool BrewProvider::is_available() const {
    return command_exists("brew");
}

SearchResult BrewProvider::search(const std::string& query) const {
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

    // Get installed packages for status
    auto installed = get_installed_packages();

    // Track if we found exact match
    std::string exact_match_name;
    std::string exact_match_desc;

    // First, check if an exact package exists (brew search often misses exact matches)
    std::string info_cmd = "brew info '" + escaped_query + "' 2>/dev/null";
    std::string info_output = exec_command(info_cmd);
    if (!info_output.empty() && info_output.find("Error:") == std::string::npos) {
        // Parse first line for name and description
        // Format: "==> name: stable X.Y.Z (bottled)"
        std::istringstream info_stream(info_output);
        std::string first_line;
        if (std::getline(info_stream, first_line) && !first_line.empty()) {
            // Strip "==> " prefix if present
            if (first_line.find("==> ") == 0) {
                first_line = first_line.substr(4);
            }
            size_t colon = first_line.find(": ");
            if (colon != std::string::npos) {
                exact_match_name = first_line.substr(0, colon);
                // Description is on a later line, version info is after colon
                // Skip the version line, try to get actual description from line 2+
            }
        }
        // Try to get description from subsequent lines (skip empty and version lines)
        std::string desc_line;
        while (std::getline(info_stream, desc_line)) {
            // Skip empty lines and lines starting with common non-description prefixes
            if (desc_line.empty()) continue;
            if (desc_line[0] == '=') continue;
            if (desc_line.find("http") == 0) continue;
            if (desc_line.find("Installed") == 0) continue;
            if (desc_line.find("From:") == 0) continue;
            if (desc_line.find("License:") == 0) continue;
            // This should be the description
            exact_match_desc = desc_line;
            break;
        }
    }

    // brew search --desc returns: "name: description"
    std::string cmd = "brew search --desc '" + escaped_query + "' 2>/dev/null";
    std::string output = exec_command(cmd);

    // Add exact match first if found and not empty
    if (!exact_match_name.empty()) {
        Package pkg;
        pkg.name = exact_match_name;
        pkg.description = exact_match_desc;
        pkg.source = "formula";
        pkg.installed = (installed.find(pkg.name) != installed.end());
        result.packages.push_back(pkg);
    }

    if (output.empty()) {
        return result;
    }

    std::istringstream stream(output);
    std::string line;
    std::string current_source = "formula";

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        // Check for section headers
        if (line.find("==> Formulae") != std::string::npos) {
            current_source = "formula";
            continue;
        }
        if (line.find("==> Casks") != std::string::npos) {
            current_source = "cask";
            continue;
        }

        // Skip other header lines
        if (line[0] == '=' || line.find("No ") == 0) continue;

        Package pkg;
        pkg.source = current_source;

        // Format with --desc: "name: description"
        size_t colon = line.find(": ");
        if (colon != std::string::npos) {
            pkg.name = line.substr(0, colon);
            pkg.description = line.substr(colon + 2);
        } else {
            // No description, just name
            pkg.name = line;
            // Trim whitespace
            while (!pkg.name.empty() && isspace(pkg.name.back())) {
                pkg.name.pop_back();
            }
        }

        if (pkg.name.empty()) continue;

        // Skip if this is the exact match we already added
        if (pkg.name == exact_match_name) continue;

        pkg.installed = (installed.find(pkg.name) != installed.end());
        result.packages.push_back(pkg);
    }

    // Sort by relevance (exact match already at top will stay there due to sort stability)
    sort_by_relevance(result.packages, query);

    return result;
}

std::string BrewProvider::install_command(const Package& pkg) const {
    if (pkg.source == "cask") {
        return "brew install --cask " + pkg.name;
    }
    return "brew install " + pkg.name;
}

} // namespace paclook
