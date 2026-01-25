#pragma once

#include "provider.hpp"

namespace paclook {

class DnfProvider : public Provider {
public:
    std::string name() const override { return "dnf"; }
    bool is_available() const override;
    SearchResult search(const std::string& query) const override;
    std::string install_command(const Package& pkg) const override;

    std::string source_color(const std::string& source) const override {
        if (source == "fedora") return "\033[34m";       // blue
        if (source == "updates") return "\033[32m";      // green
        if (source == "@System") return "\033[36m";      // cyan
        return "\033[33m";                               // yellow
    }
};

} // namespace paclook
