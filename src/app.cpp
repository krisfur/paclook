#include "app.hpp"
#include "providers/paru.hpp"
#include "providers/yay.hpp"
#include "providers/pacman.hpp"
#include "providers/xbps.hpp"
#include "providers/apt.hpp"
#include "providers/dnf.hpp"
#include "providers/apk.hpp"
#include "providers/brew.hpp"
#include <algorithm>
#include <sstream>

namespace paclook {

// Factory function to create providers
ProviderPtr create_provider(const std::string& name) {
    if (name == "paru") {
        return std::make_unique<ParuProvider>();
    } else if (name == "yay") {
        return std::make_unique<YayProvider>();
    } else if (name == "pacman") {
        return std::make_unique<PacmanProvider>();
    } else if (name == "xbps") {
        return std::make_unique<XbpsProvider>();
    } else if (name == "apt") {
        return std::make_unique<AptProvider>();
    } else if (name == "dnf") {
        return std::make_unique<DnfProvider>();
    } else if (name == "apk") {
        return std::make_unique<ApkProvider>();
    } else if (name == "brew") {
        return std::make_unique<BrewProvider>();
    }
    return nullptr;
}

std::vector<std::string> get_available_providers() {
    std::vector<std::string> available;

    // Check each provider in order of preference
    auto paru = std::make_unique<ParuProvider>();
    if (paru->is_available()) {
        available.push_back("paru");
    }

    auto yay = std::make_unique<YayProvider>();
    if (yay->is_available()) {
        available.push_back("yay");
    }

    auto pacman = std::make_unique<PacmanProvider>();
    if (pacman->is_available()) {
        available.push_back("pacman");
    }

    auto xbps = std::make_unique<XbpsProvider>();
    if (xbps->is_available()) {
        available.push_back("xbps");
    }

    auto brew = std::make_unique<BrewProvider>();
    if (brew->is_available()) {
        available.push_back("brew");
    }

    auto dnf = std::make_unique<DnfProvider>();
    if (dnf->is_available()) {
        available.push_back("dnf");
    }

    auto apk = std::make_unique<ApkProvider>();
    if (apk->is_available()) {
        available.push_back("apk");
    }

    auto apt = std::make_unique<AptProvider>();
    if (apt->is_available()) {
        available.push_back("apt");
    }

    return available;
}

App::App()
    : last_input_time_(std::chrono::steady_clock::now()) {
}

App::~App() {
    terminal_.restore();
    terminal_.show_cursor();
}

bool App::init(const std::string& provider_name) {
    // If provider specified, try to use it
    if (!provider_name.empty()) {
        provider_ = create_provider(provider_name);
        if (provider_ && provider_->is_available()) {
            return true;
        }
        status_message_ = "Provider '" + provider_name + "' not available";
        return false;
    }

    // Auto-detect provider
    auto available = get_available_providers();
    if (available.empty()) {
        status_message_ = "No supported package manager found";
        return false;
    }

    // Use first available (in order of preference)
    provider_ = create_provider(available[0]);
    return provider_ != nullptr;
}

void App::run() {
    terminal_.setup_raw_mode();
    terminal_.hide_cursor();
    terminal_.clear_screen();

    status_message_ = "Start typing to search.";

    while (!should_quit_) {
        // Check if we need to perform a debounced search
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_input_time_).count();

        if (needs_search_) {
            if (elapsed >= DEBOUNCE_MS) {
                search();
                needs_search_ = false;
            } else {
                // Show searching indicator while waiting for debounce
                status_message_ = "Searching...";
            }
        }

        draw();

        int key = terminal_.read_key();
        if (key != Terminal::KEY_NONE) {
            handle_input(key);
        }
    }

    // Clear screen before exit
    terminal_.clear_screen();
    terminal_.restore();
    terminal_.show_cursor();
}

void App::draw() {
    terminal_.update_size();

    std::ostringstream output;

    // Move to home position and clear screen
    output << "\033[H\033[J";

    int total_packages = static_cast<int>(packages_.size());

    // Calculate how many packages to display (up to VISIBLE_ITEMS)
    int display_count = std::min(VISIBLE_ITEMS, total_packages - scroll_offset_);
    if (display_count < 0) display_count = 0;

    // Draw packages in REVERSE order (index 0 at bottom, near search box)
    // This means we draw from highest visible index to lowest
    for (int i = 0; i < display_count; ++i) {
        // Calculate which package to show at this position
        // Position 0 (top) shows the highest index in visible range
        // Position display_count-1 (bottom) shows the lowest index (scroll_offset_)
        int pkg_idx = scroll_offset_ + display_count - 1 - i;

        if (pkg_idx < 0 || pkg_idx >= total_packages) continue;

        const auto& pkg = packages_[pkg_idx];
        bool is_selected = (pkg_idx == selected_index_);

        // Line 1: [source] > name version
        if (is_selected) {
            output << Terminal::REVERSE;
        }

        // Source tag with color
        output << provider_->source_color(pkg.source);
        output << "[" << pkg.source << "]" << Terminal::RESET;

        if (is_selected) {
            output << Terminal::REVERSE;
        }

        // Installed indicator
        if (pkg.installed) {
            output << Terminal::GREEN << " *" << Terminal::RESET;
            if (is_selected) output << Terminal::REVERSE;
        } else {
            output << "  ";
        }

        // Package name (bold)
        output << " " << Terminal::BOLD << pkg.name << Terminal::RESET;
        if (is_selected) output << Terminal::REVERSE;

        // Version
        output << " " << pkg.version;

        output << Terminal::RESET << "\033[K\r\n";

        // Line 2: description (indented)
        if (is_selected) {
            output << Terminal::REVERSE;
        }
        output << "         " << truncate(pkg.description, DESC_MAX_LEN);
        output << Terminal::RESET << "\033[K\r\n";
    }

    // Provider indicator + status message
    output << Terminal::DIM << "[" << provider_->name() << "]" << Terminal::RESET << " ";

    // Status message with color based on content
    if (status_message_.find("Found") != std::string::npos) {
        output << Terminal::GREEN;
    } else if (status_message_.find("Searching") != std::string::npos) {
        output << Terminal::YELLOW;
    } else if (status_message_.find("Too many") != std::string::npos ||
               status_message_.find("Error") != std::string::npos ||
               status_message_.find("No results") != std::string::npos) {
        output << Terminal::RED;
    } else {
        output << Terminal::DIM;
    }
    output << status_message_ << Terminal::RESET << "\033[K\r\n";

    // Separator
    output << Terminal::DIM;
    output << "──────────────────────────────────────────────────────────────────";
    output << Terminal::RESET << "\033[K\r\n";

    // Info line
    output << "Results: " << total_packages;
    output << "  |  ↑↓: navigate  |  Enter: install  |  Ctrl+X: quit";
    output << "\033[K\r\n";

    // Search box
    output << Terminal::BOLD << "Search: " << Terminal::RESET;
    output << search_query_;

    terminal_.write(output.str());
    terminal_.flush();
}

void App::search() {
    if (search_query_.empty()) {
        packages_.clear();
        selected_index_ = 0;
        scroll_offset_ = 0;
        status_message_ = "Start typing to search.";
        return;
    }

    auto result = provider_->search(search_query_);
    packages_ = std::move(result.packages);

    if (result.has_error()) {
        status_message_ = result.error;
    } else if (packages_.empty()) {
        status_message_ = "No results found.";
    } else {
        int count = static_cast<int>(packages_.size());
        status_message_ = "Found " + std::to_string(count) + " result" +
                         (count == 1 ? "." : "s.");
    }

    // Selection starts at index 0 (bottom-most visible result)
    selected_index_ = 0;
    scroll_offset_ = 0;
}

void App::handle_input(int key) {
    switch (key) {
        case Terminal::KEY_CTRL_X:
        case Terminal::KEY_CTRL_Q:
        case Terminal::KEY_CTRL_C:
            should_quit_ = true;
            break;

        case Terminal::KEY_ENTER:
            if (!packages_.empty()) {
                install_selected();
            }
            break;

        case Terminal::KEY_UP:
            // In reverse display: UP moves to higher index (next result up visually)
            if (selected_index_ < static_cast<int>(packages_.size()) - 1) {
                selected_index_++;
                // Adjust scroll if needed (selected went above visible area)
                if (selected_index_ >= scroll_offset_ + VISIBLE_ITEMS) {
                    scroll_offset_ = selected_index_ - VISIBLE_ITEMS + 1;
                }
            }
            break;

        case Terminal::KEY_DOWN:
            // In reverse display: DOWN moves to lower index (next result down visually)
            if (selected_index_ > 0) {
                selected_index_--;
                // Adjust scroll if needed (selected went below visible area)
                if (selected_index_ < scroll_offset_) {
                    scroll_offset_ = selected_index_;
                }
            }
            break;

        case Terminal::KEY_PAGE_UP: {
            int max_idx = static_cast<int>(packages_.size()) - 1;
            selected_index_ = std::min(max_idx, selected_index_ + VISIBLE_ITEMS);
            int max_scroll = std::max(0, static_cast<int>(packages_.size()) - VISIBLE_ITEMS);
            scroll_offset_ = std::min(max_scroll, scroll_offset_ + VISIBLE_ITEMS);
            break;
        }

        case Terminal::KEY_PAGE_DOWN:
            selected_index_ = std::max(0, selected_index_ - VISIBLE_ITEMS);
            scroll_offset_ = std::max(0, scroll_offset_ - VISIBLE_ITEMS);
            break;

        case Terminal::KEY_HOME:
            // Go to last package (top of list visually)
            if (!packages_.empty()) {
                selected_index_ = static_cast<int>(packages_.size()) - 1;
                scroll_offset_ = std::max(0, static_cast<int>(packages_.size()) - VISIBLE_ITEMS);
            }
            break;

        case Terminal::KEY_END:
            // Go to first package (bottom of list visually)
            selected_index_ = 0;
            scroll_offset_ = 0;
            break;

        case Terminal::KEY_BACKSPACE:
            if (!search_query_.empty()) {
                search_query_.pop_back();
                needs_search_ = true;
                last_input_time_ = std::chrono::steady_clock::now();
            }
            break;

        case Terminal::KEY_ESCAPE:
            // Clear search
            search_query_.clear();
            packages_.clear();
            selected_index_ = 0;
            scroll_offset_ = 0;
            status_message_ = "Start typing to search.";
            break;

        default:
            // Regular character input
            if (key >= 32 && key < 127) {
                search_query_ += static_cast<char>(key);
                needs_search_ = true;
                last_input_time_ = std::chrono::steady_clock::now();
            }
            break;
    }
}

void App::install_selected() {
    if (packages_.empty() || selected_index_ >= static_cast<int>(packages_.size())) {
        return;
    }

    const auto& pkg = packages_[selected_index_];
    std::string cmd = provider_->install_command(pkg);

    // Restore terminal before running command
    terminal_.restore();
    terminal_.show_cursor();
    terminal_.clear_screen();

    // Print what we're doing
    printf("\nInstalling %s from %s...\n\n", pkg.name.c_str(), pkg.source.c_str());

    // Run the install command
    int result = system(cmd.c_str());

    // Wait for user to press enter
    printf("\n%sPress Enter to continue...%s", Terminal::DIM, Terminal::RESET);
    fflush(stdout);
    getchar();

    // Re-setup terminal
    terminal_.setup_raw_mode();
    terminal_.hide_cursor();
    terminal_.clear_screen();

    if (result == 0) {
        status_message_ = "Successfully installed " + pkg.name;
        // Mark as installed in our list
        packages_[selected_index_].installed = true;
    } else {
        status_message_ = "Installation of " + pkg.name + " may have failed";
    }
}

std::string App::truncate(const std::string& str, size_t max_len) const {
    if (str.length() <= max_len) {
        return str;
    }
    return str.substr(0, max_len - 3) + "...";
}

std::string App::get_status_message() const {
    return status_message_;
}

} // namespace paclook
