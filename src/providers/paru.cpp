#include "providers/paru.hpp"
#include <array>
#include <cstdio>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

namespace paclook {

namespace {

// Execute command and capture both stdout and stderr
struct ExecResult {
    std::string stdout_output;
    std::string stderr_output;
    int exit_code;
};

ExecResult exec_command_full(const std::string& cmd) {
    ExecResult result;
    result.exit_code = -1;

    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        return result;
    }

    if (pid == 0) {
        // Child process
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        _exit(127);
    }

    // Parent process
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    std::array<char, 4096> buffer;
    ssize_t n;

    while ((n = read(stdout_pipe[0], buffer.data(), buffer.size())) > 0) {
        result.stdout_output.append(buffer.data(), n);
    }
    close(stdout_pipe[0]);

    while ((n = read(stderr_pipe[0], buffer.data(), buffer.size())) > 0) {
        result.stderr_output.append(buffer.data(), n);
    }
    close(stderr_pipe[0]);

    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    }

    return result;
}

bool command_exists(const std::string& cmd) {
    std::string check = "command -v " + cmd + " > /dev/null 2>&1";
    return system(check.c_str()) == 0;
}

} // anonymous namespace

bool ParuProvider::is_available() const {
    return command_exists("paru");
}

SearchResult ParuProvider::search(const std::string& query) const {
    SearchResult result;

    if (query.empty()) {
        return result;
    }

    // Escape query for shell
    std::string escaped_query = query;
    for (size_t i = 0; i < escaped_query.size(); ++i) {
        if (escaped_query[i] == '\'' || escaped_query[i] == '"' ||
            escaped_query[i] == '\\' || escaped_query[i] == '`' ||
            escaped_query[i] == '$') {
            escaped_query.insert(i, "\\");
            ++i;
        }
    }

    std::string cmd = "paru -Ss '" + escaped_query + "' 2>&1";
    auto exec_result = exec_command_full("paru -Ss '" + escaped_query + "'");

    // Check stderr for paru-specific errors
    if (exec_result.stderr_output.find("Query arg too small") != std::string::npos ||
        exec_result.stderr_output.find("Too many package results") != std::string::npos) {
        result.error = "Too many results! Try a more specific search.";
        return result;
    }

    // Also check stdout (some versions output errors there)
    if (exec_result.stdout_output.find("Query arg too small") != std::string::npos ||
        exec_result.stdout_output.find("Too many package results") != std::string::npos) {
        result.error = "Too many results! Try a more specific search.";
        return result;
    }

    if (exec_result.stdout_output.empty()) {
        return result;  // Empty results, no error
    }

    // Parse paru output format:
    // repo/package-name version [installed]
    //     description
    std::istringstream stream(exec_result.stdout_output);
    std::string line;
    Package current;
    bool has_package = false;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        // Check if this is a package header line (starts with repo/)
        if (!line.empty() && line[0] != ' ' && line[0] != '\t') {
            // Save previous package if exists
            if (has_package) {
                result.packages.push_back(current);
                current = Package{};
            }

            // Parse: repo/name version [installed]
            size_t slash_pos = line.find('/');
            if (slash_pos == std::string::npos) continue;

            current.source = line.substr(0, slash_pos);

            size_t name_start = slash_pos + 1;
            size_t space_pos = line.find(' ', name_start);
            if (space_pos == std::string::npos) continue;

            current.name = line.substr(name_start, space_pos - name_start);

            // Find version (after space, before next space or [installed])
            size_t version_start = space_pos + 1;
            size_t version_end = line.find(' ', version_start);
            if (version_end == std::string::npos) {
                current.version = line.substr(version_start);
            } else {
                current.version = line.substr(version_start, version_end - version_start);
            }

            // Check if installed
            current.installed = (line.find("[installed]") != std::string::npos ||
                                line.find("[Installed]") != std::string::npos);

            has_package = true;
        } else if (has_package) {
            // This is a description line (indented)
            size_t desc_start = line.find_first_not_of(" \t");
            if (desc_start != std::string::npos) {
                current.description = line.substr(desc_start);
            }
        }
    }

    // Don't forget the last package
    if (has_package) {
        result.packages.push_back(current);
    }

    return result;
}

std::string ParuProvider::install_command(const Package& pkg) const {
    return "paru -S " + pkg.name;
}

} // namespace paclook
