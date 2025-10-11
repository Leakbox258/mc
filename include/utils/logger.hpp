#ifndef UTILS_LOGGER_HPP
#define UTILS_LOGGER_HPP

#include <chrono>
#include <format>
#include <iostream>
#include <mutex>
#include <source_location>
#include <string_view>
#include <utility>

namespace {

inline std::mutex log_mutex;

inline std::string get_formatted_timestamp() {
    const auto now = std::chrono::system_clock::now();
    return std::format("{:%Y-%m-%d %H:%M:%S}", now);
}

/// colors
constexpr std::string_view COLOR_RESET = "\033[0m";
constexpr std::string_view COLOR_RED = "\033[1;31m";
constexpr std::string_view COLOR_GREEN = "\033[1;32m";
constexpr std::string_view COLOR_YELLOW = "\033[1;33m";
constexpr std::string_view COLOR_BLUE = "\033[1;34m";
constexpr std::string_view COLOR_MAGENTA = "\033[1;35m";
constexpr std::string_view COLOR_CYAN = "\033[1;36m";

inline std::string colorize(std::string_view text, std::string_view color) {
    return std::string(color) + std::string(text) + std::string(COLOR_RESET);
}

} // namespace

namespace utils {
namespace logger {

template <typename... Args>
void info(std::string_view format_str, Args&&... args) {
    std::lock_guard<std::mutex> lock(log_mutex);
    try {
        std::cout << std::format(
            "[{}] [{}] {}\n", get_formatted_timestamp(),
            colorize("INFO", COLOR_CYAN),
            std::vformat(format_str, std::make_format_args(args...)));
    } catch (const std::format_error& e) {
        std::cerr << std::format("[{}] [{}] Formatting error: {}\n",
                                 get_formatted_timestamp(),
                                 colorize("LOGGER_ERROR", COLOR_RED), e.what());
    }
}

[[noreturn]] inline void unreachable(
    std::string_view message = "Unreachable code executed",
    const std::source_location& location = std::source_location::current()) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cerr << std::format(
        "[{}] [{}] {}\n"
        "  -> at {}:{}\n"
        "  -> in function: {}\n",
        get_formatted_timestamp(), colorize("UNREACHABLE", COLOR_RED), message,
        location.file_name(), location.line(), location.function_name());
    std::unreachable();
}

[[noreturn]] inline void
todo(std::string_view message = "NoImplmented code executed",
     const std::source_location& location = std::source_location::current()) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cerr << std::format(
        "[{}] [{}] {}\n"
        "  -> at {}:{}\n"
        "  -> in function: {}\n",
        get_formatted_timestamp(), colorize("TODO", COLOR_RED), message,
        location.file_name(), location.line(), location.function_name());
    std::unreachable();
}

/// assert inner
inline void assert_handler(bool condition, std::string_view expression,
                           std::string_view message,
                           const std::source_location& location) {

    if (!condition) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cerr << std::format("[{}] [{}] {}\n"
                                 "  -> Expression: {}\n"
                                 "  -> at {}:{}\n"
                                 "  -> in function: {}\n",
                                 get_formatted_timestamp(),
                                 colorize("ASSERTION FAILED", COLOR_RED),
                                 message, expression, location.file_name(),
                                 location.line(), location.function_name());

        std::abort();
    }
}

} // namespace logger

using logger::assert_handler;
using logger::info;
using logger::todo;
using logger::unreachable;

} // namespace utils

#endif