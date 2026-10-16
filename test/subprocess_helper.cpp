#include "subprocess_helper.h"

#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <string>
#include <subprocess.h>
#include <thread>
#include <vector>

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

    // Write any input, then close stdin so a command reading paths from stdin
    // sees EOF instead of waiting forever. subprocess_join closes it too, but
    // only after the output has been read, which is too late for such a command
    FILE *stdin_stream = subprocess_stdin(&process);
    if (stdin_stream != nullptr) {
        if (options.stdin_input.has_value()) {
            std::fwrite(options.stdin_input->data(), 1, options.stdin_input->size(), stdin_stream);
        }
        std::fclose(stdin_stream);
        process.stdin_file = nullptr;
    }

    RunResult result;

    // Watchdog: a hung binary is killed and reported rather than hanging the
    // whole ctest run
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

    // stderr is drained on its own thread: a process that fills one pipe while
    // the parent is blocked reading the other would otherwise deadlock
    std::thread stderr_reader([&] { result.stderr_output = ReadAll(subprocess_stderr(&process)); });
    result.stdout_output = ReadAll(subprocess_stdout(&process));
    stderr_reader.join();

    int returncode = 0;
    subprocess_join(&process, &returncode);
    {
        std::lock_guard<std::mutex> lock(finished_mutex);
        finished = true;
    }
    finished_cv.notify_all();
    watchdog.join();
    subprocess_destroy(&process);

    result.returncode = result.timed_out ? -1 : returncode;
    return result;
}

} // namespace mpqcli::testing
