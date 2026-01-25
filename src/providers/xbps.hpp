#pragma once

#include "provider.hpp"

namespace paclook {

class XbpsProvider : public Provider {
public:
    std::string name() const override { return "xbps"; }
    bool is_available() const override;
    SearchResult search(const std::string& query) const override;
    std::string install_command(const Package& pkg) const override;

    std::string source_color(const std::string& source) const override {
        // Void Linux uses a single repo, color by installed status instead
        return "\033[32m";  // green
    }
};

} // namespace paclook
