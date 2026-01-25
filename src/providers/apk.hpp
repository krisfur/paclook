#pragma once

#include "provider.hpp"

namespace paclook {

class ApkProvider : public Provider {
public:
    std::string name() const override { return "apk"; }
    bool is_available() const override;
    SearchResult search(const std::string& query) const override;
    std::string install_command(const Package& pkg) const override;

    std::string source_color(const std::string& source) const override {
        if (source == "community") return "\033[33m";  // yellow
        return "\033[34m";                             // blue for main
    }
};

} // namespace paclook
