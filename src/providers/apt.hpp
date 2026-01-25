#pragma once

#include "provider.hpp"

namespace paclook {

class AptProvider : public Provider {
public:
    std::string name() const override { return "apt"; }
    bool is_available() const override;
    SearchResult search(const std::string& query) const override;
    std::string install_command(const Package& pkg) const override;

    std::string source_color(const std::string& source) const override {
        return "\033[33m";  // yellow for debian/ubuntu
    }
};

} // namespace paclook
