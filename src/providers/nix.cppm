module;

#include <array>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>

export module paclook.providers.nix;

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

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t");
    return str.substr(start, end - start + 1);
}

// Extract package name from attribute like "nixpkgs.go" or "nixpkgs.haskellPackages.mtl"
std::string extract_pkg_name(const std::string& attr) {
    std::string path = attr;

    // Remove "nixpkgs." prefix if present
    if (path.find("nixpkgs.") == 0) {
        path = path.substr(8);
    }

    return path;
}

} // anonymous namespace

export class NixProvider : public Provider {
public:
    std::string name() const override { return "nix"; }
    bool is_available() const override;
    SearchResult search(const std::string& query) const override;
    std::string install_command(const Package& pkg) const override;

    std::string source_color(const std::string& source) const override {
        if (source == "nixpkgs") return "\033[34m";        // blue
        if (source == "nixos") return "\033[36m";          // cyan
        return "\033[35m";                                  // magenta
    }
};

bool NixProvider::is_available() const {
    return command_exists("nix") || command_exists("nix-env");
}

SearchResult NixProvider::search(const std::string& query) const {
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

    // Use nix-env -qaP --description for better name-focused search
    // Output format: "nixpkgs.go    go-1.24.9   Go Programming language"
    // Use regex pattern .*query.* for fuzzy matching
    std::string cmd = "nix-env -qaP --description '.*" + escaped_query + ".*' 2>/dev/null";
    std::string output = exec_command(cmd);

    if (!output.empty()) {
        std::istringstream stream(output);
        std::string line;

        while (std::getline(stream, line)) {
            if (line.empty()) continue;

            Package pkg;

            // Parse: "nixpkgs.go    go-1.24.9   Go Programming language"
            // Fields are separated by multiple spaces

            // Find first field (attribute)
            size_t attr_end = line.find_first_of(" \t");
            if (attr_end == std::string::npos) continue;
            std::string attr = line.substr(0, attr_end);

            // Skip whitespace to find name-version
            size_t name_start = line.find_first_not_of(" \t", attr_end);
            if (name_start == std::string::npos) continue;

            size_t name_end = line.find_first_of(" \t", name_start);
            std::string name_version;
            if (name_end == std::string::npos) {
                name_version = line.substr(name_start);
            } else {
                name_version = line.substr(name_start, name_end - name_start);

                // Rest is description
                size_t desc_start = line.find_first_not_of(" \t", name_end);
                if (desc_start != std::string::npos) {
                    pkg.description = line.substr(desc_start);
                }
            }

            if (attr.empty() || name_version.empty()) continue;

            // Extract package name from attribute (e.g., "nixpkgs.go" -> "go")
            pkg.name = extract_pkg_name(attr);
            pkg.source = "nixpkgs";

            // Try to separate version from name-version (e.g., "go-1.24.9")
            // Find last dash followed by a digit
            size_t version_start = std::string::npos;
            for (size_t i = name_version.length(); i > 0; --i) {
                if (name_version[i-1] == '-' && i < name_version.length() &&
                    std::isdigit(name_version[i])) {
                    version_start = i;
                    break;
                }
            }
            if (version_start != std::string::npos) {
                pkg.version = name_version.substr(version_start);
            }

            result.packages.push_back(pkg);
        }
    }

    // Check for exact match using nix eval
    if (!result.packages.empty()) {
        // Check if any result is an exact match - if so, it will be sorted to top
        // by sort_by_relevance
    }

    // Sort by relevance
    sort_by_relevance(result.packages, query);

    return result;
}

std::string NixProvider::install_command(const Package& pkg) const {
    return "nix-env -iA nixpkgs." + pkg.name;
}

} // namespace paclook
