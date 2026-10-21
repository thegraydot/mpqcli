#include "subprocess_helper.h"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <subprocess.h>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <cerrno>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;

namespace mpqcli::testing {

namespace {

// Drains a pipe to EOF as raw bytes. fread rather than getline so binary output,
// such as a printed signature, comes back byte for byte
std::string ReadAll(FILE *stream) {
    std::string output;
    if (stream == nullptr) {
        return output;
    }
    char buffer[4096];
    std::size_t count = 0;
    while ((count = std::fread(buffer, 1, sizeof(buffer), stream)) > 0) {
        output.append(buffer, count);
    }
    return output;
}

// Writes any input, then closes stdin so a command reading paths from stdin sees EOF
// instead of waiting forever
void WriteInput(subprocess_s &process, const std::optional<std::string> &input) {
    FILE *stdin_stream = subprocess_stdin(&process);
    if (stdin_stream == nullptr) {
        return;
    }
    if (input.has_value()) {
        std::fwrite(input->data(), 1, input->size(), stdin_stream);
    }
    std::fclose(stdin_stream);
    process.stdin_file = nullptr;
}

// Reaps the child and returns its exit code, or nothing if it cannot be waited for. On
// POSIX a child killed by a signal gives 128 plus the signal number, as a shell reports
// it. subprocess_join is no use for that there: it reports any signal as EXIT_FAILURE,
// so a crash would pass for an expected failure.
std::optional<int> WaitForExit(subprocess_s &process) {
#ifdef _WIN32
    int returncode = -1;
    if (subprocess_join(&process, &returncode) != 0) {
        return std::nullopt;
    }
    return returncode;
#else
    int status = 0;
    pid_t reaped = -1;
    do {
        reaped = waitpid(process.child, &status, 0);
    } while (reaped == -1 && errno == EINTR);
    if (reaped != process.child) {
        return std::nullopt;
    }
    return WIFSIGNALED(status) ? 128 + WTERMSIG(status) : WEXITSTATUS(status);
#endif
}

} // namespace

RunResult Run(const fs::path &binary, const std::vector<std::string> &args,
              const RunOptions &options) {
    // subprocess.h wants a NULL-terminated argv of C strings, so the std::strings
    // have to outlive the pointers taken into them
    std::vector<std::string> arguments;
    arguments.reserve(args.size() + 1);
    arguments.push_back(binary.string());
    arguments.insert(arguments.end(), args.begin(), args.end());
    std::vector<const char *> argv;
    argv.reserve(arguments.size() + 1);
    for (const auto &argument : arguments) {
        argv.push_back(argument.c_str());
    }
    argv.push_back(nullptr);

    std::string working_dir;
    const char *working_dir_ptr = nullptr;
    if (options.working_dir.has_value()) {
        working_dir = options.working_dir->string();
        working_dir_ptr = working_dir.c_str();
    }

    subprocess_s process{};
    const int spawn_options = subprocess_option_inherit_environment | subprocess_option_no_window;
    if (subprocess_create_ex(argv.data(), spawn_options, nullptr, working_dir_ptr, &process) != 0) {
        throw std::runtime_error("subprocess_helper: failed to spawn " + binary.string());
    }

    // A command that rejects its arguments exits without reading stdin, so the write
    // can meet a closed pipe.
#ifdef _WIN32
    // subprocess.h keeps the parent's copy of the child's stdin read end open until
    // join, so the pipe never closes: the write fills it and blocks forever, and the
    // watchdog cannot free it by killing a child that has already exited. Closing that
    // copy now, as POSIX does, leaves the child the only reader, so the write fails
    // once the child is gone.
    CloseHandle(process.hStdInput);
    process.hStdInput = nullptr;
#else
    // That raises SIGPIPE, whose default action kills the whole test binary rather
    // than failing the write, so it is ignored and the write simply comes up short.
    std::signal(SIGPIPE, SIG_IGN);
#endif

    RunResult result;

    // Watchdog: a hung binary is killed and reported rather than hanging the whole
    // ctest run. It starts before any I/O, so nothing below can block without a limit.
    std::mutex finished_mutex;
    std::condition_variable finished_cv;
    bool finished = false;
    std::thread watchdog([&] {
        std::unique_lock<std::mutex> lock(finished_mutex);
        if (!finished_cv.wait_for(lock, options.timeout, [&] { return finished; })) {
            result.timed_out = true;
            subprocess_terminate(&process);
        }
    });

    // Input is written beside the readers rather than before them: a child that ignores
    // its stdin and fills its stdout would otherwise block on the output while the
    // helper blocked on the input. stderr gets its own thread for the same reason.
    std::thread stdin_writer([&] { WriteInput(process, options.stdin_input); });
    std::thread stderr_reader([&] { result.stderr_output = ReadAll(subprocess_stderr(&process)); });
    result.stdout_output = ReadAll(subprocess_stdout(&process));
    stderr_reader.join();
    stdin_writer.join();

    // The watchdog stops before the child is reaped, never after: subprocess_terminate
    // signals the child's pid, which is free for reuse once the child is reaped. Both
    // pipes are closed by now, so the child has exited or is about to.
    {
        std::lock_guard<std::mutex> lock(finished_mutex);
        finished = true;
    }
    finished_cv.notify_all();
    watchdog.join();

    const std::optional<int> returncode = WaitForExit(process);
    subprocess_destroy(&process);
    if (!returncode.has_value()) {
        throw std::runtime_error("subprocess_helper: failed to wait for " + binary.string());
    }

    if (result.timed_out) {
        // Said here because a test sees only the returncode of -1
        std::cerr << "subprocess_helper: killed " << binary.string() << " after "
                  << options.timeout.count() << " s" << std::endl;
        result.returncode = -1;
    } else {
        result.returncode = *returncode;
    }
    return result;
}

} // namespace mpqcli::testing
