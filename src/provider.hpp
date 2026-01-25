#pragma once

#include "package.hpp"
#include <string>
#include <memory>

namespace paclook {

// Result of a search operation
struct SearchResult {
    PackageList packages;
    std::string error;  // Empty if no error

    bool has_error() const { return !error.empty(); }
};

// Abstract base class for package providers
class Provider {
public:
    virtual ~Provider() = default;

    // Get the name of this provider (e.g., "paru", "pacman", "apt")
    virtual std::string name() const = 0;

    // Check if this provider is available on the system
    virtual bool is_available() const = 0;

    // Search for packages matching the query
    virtual SearchResult search(const std::string& query) const = 0;

    // Install a package (returns the command to run)
    virtual std::string install_command(const Package& pkg) const = 0;

    // Get color for a source/repository
    virtual std::string source_color(const std::string& source) const {
        if (source == "core") return "\033[36m";       // cyan
        if (source == "extra") return "\033[32m";      // green
        if (source == "community") return "\033[33m";  // yellow
        if (source == "multilib") return "\033[35m";   // magenta
        if (source == "aur") return "\033[94m";        // bright blue
        return "\033[37m";                             // white (default)
    }
};

using ProviderPtr = std::unique_ptr<Provider>;

} // namespace paclook
