#pragma once

#include "provider.hpp"

namespace paclook {

class ZypperProvider : public Provider {
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

} // namespace paclook
