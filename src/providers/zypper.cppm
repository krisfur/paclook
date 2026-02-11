module;

#include <array>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

export module paclook.providers.zypper;

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

// Trim whitespace from both ends
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t");
    return str.substr(start, end - start + 1);
}

} // anonymous namespace

export class ZypperProvider : public Provider {
public:
    std::string name() const override { return "zypper"; }
    bool is_available() const override;
    SearchResult search(const std::string& query) const override;
    std::string install_command(const Package& pkg) const override;

    std::string source_color(const std::string& source) const override {
        if (source == "repo-oss") return "\033[32m";           // green
        if (source == "repo-non-oss") return "\033[33m";       // yellow
        if (source == "repo-update") return "\033[34m";        // blue
        if (source == "repo-update-non-oss") return "\033[35m"; // magenta
        return "\033[36m";                                      // cyan
    }
};

bool ZypperProvider::is_available() const {
    return command_exists("zypper");
}

SearchResult ZypperProvider::search(const std::string& query) const {
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

    // First, check if an exact package exists
    Package exact_match;
    bool has_exact = false;
    std::string info_cmd = "zypper --quiet info '" + escaped_query + "' 2>/dev/null";
    std::string info_output = exec_command(info_cmd);

    if (!info_output.empty() && info_output.find("not found") == std::string::npos) {
        std::istringstream info_stream(info_output);
        std::string info_line;
        while (std::getline(info_stream, info_line)) {
            if (info_line.find("Repository") == 0) {
                size_t colon = info_line.find(':');
                if (colon != std::string::npos) {
                    exact_match.source = trim(info_line.substr(colon + 1));
                }
            } else if (info_line.find("Name") == 0) {
                size_t colon = info_line.find(':');
                if (colon != std::string::npos) {
                    exact_match.name = trim(info_line.substr(colon + 1));
                }
            } else if (info_line.find("Version") == 0) {
                size_t colon = info_line.find(':');
                if (colon != std::string::npos) {
                    exact_match.version = trim(info_line.substr(colon + 1));
                }
            } else if (info_line.find("Summary") == 0) {
                size_t colon = info_line.find(':');
                if (colon != std::string::npos) {
                    exact_match.description = trim(info_line.substr(colon + 1));
                }
            } else if (info_line.find("Installed") == 0) {
                exact_match.installed = (info_line.find("Yes") != std::string::npos);
            }
        }
        if (!exact_match.name.empty()) {
            has_exact = true;
        }
    }

    // zypper search output format:
    // S  | Name         | Summary                    | Type
    // ---+--------------+----------------------------+--------
    //    | package-name | Description                | package
    // i  | installed    | Description                | package
    std::string cmd = "zypper --quiet search '" + escaped_query + "' 2>/dev/null";
    std::string output = exec_command(cmd);

    if (output.empty() && !has_exact) {
        return result;
    }

    std::istringstream stream(output);
    std::string line;
    bool past_header = false;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        // Skip header line and separator
        if (line.find("Name") != std::string::npos && line.find("Summary") != std::string::npos) {
            continue;
        }
        if (line.find("---") == 0 || line.find("--+") != std::string::npos) {
            past_header = true;
            continue;
        }

        // Parse data rows: "S | Name | Summary | Type"
        // Split by |
        std::vector<std::string> fields;
        std::istringstream line_stream(line);
        std::string field;
        while (std::getline(line_stream, field, '|')) {
            fields.push_back(trim(field));
        }

        if (fields.size() < 3) continue;

        Package pkg;

        // Field 0: Status (i = installed, empty = not installed)
        pkg.installed = (fields[0] == "i" || fields[0] == "i+");

        // Field 1: Name
        pkg.name = fields[1];
        if (pkg.name.empty()) continue;

        // Field 2: Summary/Description
        pkg.description = fields[2];

        // Field 3: Type (optional, use as source indicator)
        if (fields.size() > 3) {
            pkg.source = fields[3];
        } else {
            pkg.source = "zypper";
        }

        result.packages.push_back(pkg);
    }

    // Add exact match at the beginning if found and not already in results
    if (has_exact) {
        bool already_exists = false;
        for (const auto& pkg : result.packages) {
            if (pkg.name == exact_match.name) {
                already_exists = true;
                break;
            }
        }
        if (!already_exists) {
            result.packages.insert(result.packages.begin(), exact_match);
        }
    }

    // Sort by relevance
    sort_by_relevance(result.packages, query);

    return result;
}

std::string ZypperProvider::install_command(const Package& pkg) const {
    return "sudo zypper install " + pkg.name;
}

} // namespace paclook
