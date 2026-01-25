# paclook

A universal interactive package CLI search tool.

![screenshot](./screenshot.png)

## Supported Providers

- **paru** - AUR helper (official repos + AUR)
- **yay** - AUR helper (official repos + AUR)
- **pacman** - Official Arch Linux repos
- **xbps** - Void Linux package manager
- **apt** - Debian/Ubuntu package manager
- **dnf** - Fedora/RHEL package manager
- apk - Alpine Linux package manager
 

More providers (homebrew, etc.) can be added easily.

## Building from source

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

For static linking (portable binary):
```bash
cmake -DSTATIC_LIBC=ON ..
make -j$(nproc)
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
./docker/test.sh void      # Void Linux (xbps)
./docker/test.sh arch      # Arch Linux (yay, pacman)
./docker/test.sh ubuntu    # Ubuntu (apt)
./docker/test.sh fedora    # Fedora (dnf)
./docker/test.sh alpine    # Alpine (apk)
```

## Adding New Providers

1. Create `src/providers/yourprovider.hpp` and `.cpp`
2. Inherit from `Provider` base class
3. Implement: `name()`, `is_available()`, `search()`, `install_command()`
4. Register in `app.cpp`: `create_provider()` and `get_available_providers()`
