#pragma once

#include <string>
#include <termios.h>

namespace paclook {

class Terminal {
public:
    Terminal();
    ~Terminal();

    // Disable copy
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    // Setup/restore terminal state
    void setup_raw_mode();
    void restore();

    // Screen operations
    void clear_screen();
    void move_cursor(int row, int col);
    void hide_cursor();
    void show_cursor();

    // Input handling (non-blocking)
    int read_key();

    // Output helpers
    void write(const std::string& text);
    void write_line(const std::string& text);
    void flush();

    // Get terminal dimensions
    int rows() const { return rows_; }
    int cols() const { return cols_; }
    void update_size();

    // ANSI color codes
    static constexpr const char* RESET = "\033[0m";
    static constexpr const char* BOLD = "\033[1m";
    static constexpr const char* DIM = "\033[2m";
    static constexpr const char* REVERSE = "\033[7m";

    static constexpr const char* BLACK = "\033[30m";
    static constexpr const char* RED = "\033[31m";
    static constexpr const char* GREEN = "\033[32m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* BLUE = "\033[34m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* CYAN = "\033[36m";
    static constexpr const char* WHITE = "\033[37m";

    static constexpr const char* BRIGHT_BLACK = "\033[90m";
    static constexpr const char* BRIGHT_RED = "\033[91m";
    static constexpr const char* BRIGHT_GREEN = "\033[92m";
    static constexpr const char* BRIGHT_YELLOW = "\033[93m";
    static constexpr const char* BRIGHT_BLUE = "\033[94m";
    static constexpr const char* BRIGHT_MAGENTA = "\033[95m";
    static constexpr const char* BRIGHT_CYAN = "\033[96m";
    static constexpr const char* BRIGHT_WHITE = "\033[97m";

    // Special key codes
    enum Key {
        KEY_NONE = -1,
        KEY_ENTER = 13,
        KEY_ESCAPE = 27,
        KEY_BACKSPACE = 127,
        KEY_TAB = 9,
        KEY_CTRL_C = 3,
        KEY_CTRL_X = 24,
        KEY_CTRL_Q = 17,
        KEY_UP = 1000,
        KEY_DOWN = 1001,
        KEY_LEFT = 1002,
        KEY_RIGHT = 1003,
        KEY_HOME = 1004,
        KEY_END = 1005,
        KEY_PAGE_UP = 1006,
        KEY_PAGE_DOWN = 1007,
        KEY_DELETE = 1008,
    };

private:
    struct termios original_termios_;
    bool raw_mode_enabled_ = false;
    int rows_ = 24;
    int cols_ = 80;
};

} // namespace paclook
