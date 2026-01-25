#include "providers/brew.hpp"
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

    // brew search --desc returns: "name: description"
    std::string cmd = "brew search --desc '" + escaped_query + "' 2>/dev/null";
    std::string output = exec_command(cmd);

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

        pkg.installed = (installed.find(pkg.name) != installed.end());
        result.packages.push_back(pkg);
    }

    // Sort by relevance
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
