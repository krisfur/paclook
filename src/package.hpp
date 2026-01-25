#pragma once

#include <string>
#include <vector>

namespace paclook {

struct Package {
    std::string name;
    std::string version;
    std::string description;
    std::string source;      // e.g., "core", "extra", "aur", "community"
    bool installed = false;
};

using PackageList = std::vector<Package>;

} // namespace paclook
