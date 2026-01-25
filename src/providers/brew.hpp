#pragma once

#include "provider.hpp"

namespace paclook {

class BrewProvider : public Provider {
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

} // namespace paclook
