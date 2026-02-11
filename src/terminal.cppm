module;

#include <string>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdio>

export module paclook.terminal;

export namespace paclook {

class Terminal {
public:
    Terminal() {
        update_size();
    }

    ~Terminal() {
        if (raw_mode_enabled_) {
            restore();
        }
    }

    // Disable copy
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    // Setup/restore terminal state
    void setup_raw_mode() {
        if (raw_mode_enabled_) return;

        // Save original terminal attributes
        tcgetattr(STDIN_FILENO, &original_termios_);

        struct termios raw = original_termios_;

        // Input modes: no break, no CR to NL, no parity check, no strip char,
        // no start/stop output control
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

        // Output modes: disable post processing
        raw.c_oflag &= ~(OPOST);

        // Control modes: set 8 bit chars
        raw.c_cflag |= (CS8);

        // Local modes: no echo, no canonical mode, no extended functions,
        // no signal chars (^Z, ^C)
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

        // Control chars: set return condition (min bytes and timeout)
        raw.c_cc[VMIN] = 0;   // Return immediately with any available bytes
        raw.c_cc[VTIME] = 1;  // 100ms timeout (for non-blocking reads)

        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        raw_mode_enabled_ = true;
    }

    void restore() {
        if (!raw_mode_enabled_) return;

        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios_);
        raw_mode_enabled_ = false;
        show_cursor();
    }

    // Screen operations
    void clear_screen() {
        write("\033[2J");      // Clear entire screen
        write("\033[H");       // Move cursor to home position
    }

    void move_cursor(int row, int col) {
        char buf[32];
        snprintf(buf, sizeof(buf), "\033[%d;%dH", row, col);
        write(buf);
    }

    void hide_cursor() {
        write("\033[?25l");
    }

    void show_cursor() {
        write("\033[?25h");
    }

    // Input handling (non-blocking)
    int read_key() {
        char c;
        ssize_t nread = read(STDIN_FILENO, &c, 1);

        if (nread <= 0) {
            return KEY_NONE;
        }

        // Handle escape sequences
        if (c == '\033') {
            char seq[3];

            if (read(STDIN_FILENO, &seq[0], 1) != 1) return KEY_ESCAPE;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) return KEY_ESCAPE;

            if (seq[0] == '[') {
                if (seq[1] >= '0' && seq[1] <= '9') {
                    if (read(STDIN_FILENO, &seq[2], 1) != 1) return KEY_ESCAPE;
                    if (seq[2] == '~') {
                        switch (seq[1]) {
                            case '1': return KEY_HOME;
                            case '3': return KEY_DELETE;
                            case '4': return KEY_END;
                            case '5': return KEY_PAGE_UP;
                            case '6': return KEY_PAGE_DOWN;
                            case '7': return KEY_HOME;
                            case '8': return KEY_END;
                        }
                    }
                } else {
                    switch (seq[1]) {
                        case 'A': return KEY_UP;
                        case 'B': return KEY_DOWN;
                        case 'C': return KEY_RIGHT;
                        case 'D': return KEY_LEFT;
                        case 'H': return KEY_HOME;
                        case 'F': return KEY_END;
                    }
                }
            } else if (seq[0] == 'O') {
                switch (seq[1]) {
                    case 'H': return KEY_HOME;
                    case 'F': return KEY_END;
                }
            }

            return KEY_ESCAPE;
        }

        // Handle backspace (some terminals send 127, some send 8)
        if (c == 127 || c == 8) {
            return KEY_BACKSPACE;
        }

        return static_cast<int>(c);
    }

    // Output helpers
    void write(const std::string& text) {
        ::write(STDOUT_FILENO, text.c_str(), text.size());
    }

    void write_line(const std::string& text) {
        write(text);
        write("\r\n");
    }

    void flush() {
        fflush(stdout);
    }

    // Get terminal dimensions
    int rows() const { return rows_; }
    int cols() const { return cols_; }

    void update_size() {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
            rows_ = ws.ws_row;
            cols_ = ws.ws_col;
        }
    }

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
