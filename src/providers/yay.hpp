#pragma once

#include "provider.hpp"

namespace paclook {

class YayProvider : public Provider {
public:
    std::string name() const override { return "yay"; }
    bool is_available() const override;
    SearchResult search(const std::string& query) const override;
    std::string install_command(const Package& pkg) const override;
};

} // namespace paclook
