#pragma once

#include "terminal.hpp"
#include "provider.hpp"
#include "package.hpp"
#include <chrono>
#include <string>
#include <vector>

namespace paclook {

class App {
public:
    App();
    ~App();

    // Initialize with a specific provider, or auto-detect
    bool init(const std::string& provider_name = "");

    // Main event loop
    void run();

private:
    // Core methods
    void draw();
    void search();
    void handle_input(int key);
    void install_selected();

    // Helper methods
    std::string truncate(const std::string& str, size_t max_len) const;
    std::string get_status_message() const;

    // Components
    Terminal terminal_;
    ProviderPtr provider_;

    // State
    std::string search_query_;
    PackageList packages_;
    int selected_index_ = 0;
    int scroll_offset_ = 0;
    std::string status_message_;
    bool needs_search_ = false;
    bool should_quit_ = false;

    // Timing for debounce
    std::chrono::steady_clock::time_point last_input_time_;
    static constexpr int DEBOUNCE_MS = 400;  // 400ms debounce delay

    // Display settings
    static constexpr int VISIBLE_ITEMS = 10;
    static constexpr int DESC_MAX_LEN = 62;
};

// Factory function to create providers
ProviderPtr create_provider(const std::string& name);

// Get list of available providers
std::vector<std::string> get_available_providers();

} // namespace paclook
