#pragma once

#include "provider.hpp"

namespace paclook {

class NixProvider : public Provider {
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

} // namespace paclook
