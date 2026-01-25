#include "terminal.hpp"
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdio>

namespace paclook {

Terminal::Terminal() {
    update_size();
}

Terminal::~Terminal() {
    if (raw_mode_enabled_) {
        restore();
    }
}

void Terminal::setup_raw_mode() {
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

void Terminal::restore() {
    if (!raw_mode_enabled_) return;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios_);
    raw_mode_enabled_ = false;
    show_cursor();
}

void Terminal::clear_screen() {
    write("\033[2J");      // Clear entire screen
    write("\033[H");       // Move cursor to home position
}

void Terminal::move_cursor(int row, int col) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\033[%d;%dH", row, col);
    write(buf);
}

void Terminal::hide_cursor() {
    write("\033[?25l");
}

void Terminal::show_cursor() {
    write("\033[?25h");
}

int Terminal::read_key() {
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

void Terminal::write(const std::string& text) {
    ::write(STDOUT_FILENO, text.c_str(), text.size());
}

void Terminal::write_line(const std::string& text) {
    write(text);
    write("\r\n");
}

void Terminal::flush() {
    fflush(stdout);
}

void Terminal::update_size() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        rows_ = ws.ws_row;
        cols_ = ws.ws_col;
    }
}

} // namespace paclook
