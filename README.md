# paclook

A universal interactive package CLI search tool.

![screenshot](./screenshot.png)

## Supported Providers

- **apk** - Alpine Linux package manager
- **apt** - Debian/Ubuntu package manager
- **brew** - Homebrew (macOS/Linux)
- **dnf** - Fedora/RHEL package manager
- **nix** - Nix/NixOS package manager
- **pacman** - Official Arch Linux repos
- **paru** - AUR helper (official repos + AUR)
- **xbps** - Void Linux package manager
- **yay** - AUR helper (official repos + AUR)
- **zypper** - openSUSE package manager


More providers can be added in a modular manner.

> NOTE: On some systems if you have not used your package manager in a while (or ever) the first search might take some time as your package manager will update its own cache.

## Building from source

Requires **clang++ >= 19** (C++26 modules support), **CMake >= 3.30**, and **Ninja**.

```bash
cmake -G Ninja -S . -B build \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

For static linking (portable binary):
```bash
cmake -G Ninja -S . -B build \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DSTATIC_LIBC=ON
cmake --build build
```

## Usage

```bash
./paclook                   # Auto-detect best available provider
./paclook -p pacman         # Use specific provider
./paclook -l                # List available providers
./paclook -h                # Show help
```

## Controls

| Key | Action |
|-----|--------|
| Type | Search for packages |
| Up/Down | Navigate results |
| PgUp/PgDn | Navigate by page |
| Enter | Install selected package |
| Escape | Clear search |
| Ctrl+X | Quit |

## Testing with Docker

First install docker for your specific machine, on Arch:

```bash
sudo pacman -S docker
sudo systemctl enable --now docker
sudo usermod -aG docker $USER
newgrp docker  # or logout/login for group to take effect
```

Then run the docker testing scripts to start an interactive session with `paclook` built from source:

```bash
./docker/test.sh alpine    # Alpine (apk)
./docker/test.sh arch      # Arch Linux (pacman, paru, yay)
./docker/test.sh fedora    # Fedora (dnf)
./docker/test.sh homebrew  # Linuxbrew (brew)
./docker/test.sh nixos     # NixOS (nix)
./docker/test.sh opensuse  # openSUSE (zypper)
./docker/test.sh ubuntu    # Ubuntu (apt)
./docker/test.sh void      # Void Linux (xbps)
```

## Adding New Providers

1. Create `src/providers/yourprovider.cppm` as a module (`export module paclook.providers.yourprovider;`)
2. Import `paclook.provider` (and `paclook.util` if you need `sort_by_relevance`)
3. Implement: `name()`, `is_available()`, `search()`, `install_command()`
4. Register in `src/app.cppm`: `create_provider()` and `get_available_providers()`
5. Add the `.cppm` file to the `FILE_SET CXX_MODULES` list in `CMakeLists.txt`
