#include <cstdio>
#include <cstring>
#include <string>

import paclook.app;

void print_help(const char* program_name) {
    printf("paclook - Universal interactive package search tool\n\n");
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  -p, --provider <name>   Use specific package provider\n");
    printf("  -l, --list              List available providers\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -v, --version           Show version\n\n");
    printf("Supported providers:\n");
    printf("  apk       - Alpine Linux package manager\n");
    printf("  apt       - Debian/Ubuntu package manager\n");
    printf("  brew      - Homebrew (macOS/Linux)\n");
    printf("  dnf       - Fedora/RHEL package manager\n");
    printf("  nix       - Nix/NixOS package manager\n");
    printf("  pacman    - Official Arch Linux repos only\n");
    printf("  paru      - AUR helper (includes official repos + AUR)\n");
    printf("  xbps      - Void Linux package manager\n");
    printf("  yay       - AUR helper (includes official repos + AUR)\n");
    printf("  zypper    - openSUSE package manager\n\n");
    printf("Controls:\n");
    printf("  Type       - Search for packages\n");
    printf("  Up/Down    - Navigate results\n");
    printf("  PgUp/PgDn  - Navigate by page\n");
    printf("  Enter      - Install selected package\n");
    printf("  Escape     - Clear search\n");
    printf("  Ctrl+X/Q   - Quit\n");
}

void print_version() {
    printf("paclook version 0.9.2\n");
}

void list_providers() {
    printf("Available package providers:\n\n");

    auto available = paclook::get_available_providers();

    if (available.empty()) {
        printf("  No supported package managers found on this system.\n\n");
        printf("Supported providers (not found):\n");
        printf("  paru   - Install with: pacman -S paru\n");
        printf("  pacman - Standard Arch Linux package manager\n");
        return;
    }

    for (const auto& name : available) {
        printf("  %s\n", name.c_str());
    }
}

int main(int argc, char* argv[]) {
    std::string provider_name;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        }
        if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0) {
            list_providers();
            return 0;
        }
        if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--provider") == 0) && i + 1 < argc) {
            provider_name = argv[++i];
        }
    }

    paclook::App app;

    if (!app.init(provider_name)) {
        fprintf(stderr, "Error: Failed to initialize paclook\n");
        fprintf(stderr, "Run '%s --list' to see available providers\n", argv[0]);
        return 1;
    }

    app.run();

    return 0;
}
