#pragma once

#include <concepts>
#include <fstream>
#include <iostream>
#include <format>
#include <source_location>

// https://github.com/archibate/minilog

namespace exlib {

#define ZSTLLOG_FOREACH_LOGLEVEL(f) \
    f(trace)\
    f(debug)\
    f(info)\
    f(warning)\
    f(error)\
    f(fatal)\

    enum class log_level : std::uint8_t{
#define _FUNCTION(name) name,
        ZSTLLOG_FOREACH_LOGLEVEL(_FUNCTION)
#undef _FUNCTION
    };

    namespace _details {

#if defined (__linux)
        inline char k_level_ansi_colors[(std::uint8_t)log_level::fatal + 1][8] = {
            "\E[37m", // trace
            "\E[35m", // debug
            "\E[32m", // info
            "\E[33m", // warning
            "\E[31m", // error 
            "\E[31;1m"    // fatal
        };

        inline char k_reset_ansi_color[4] = "\E[m";

#define ZSTLLOG_IF_HAS_ANSI_COLORS(x) x
#else
#define ZSTLLOG_IF_HAS_ANSI_COLORS(x)
#endif

        inline log_level max_level = log_level::info;

        inline std::string log_level_name(log_level lev) {
            switch (lev) {
#define _FUNCTION(name) case log_level::name: return #name;
    ZSTLLOG_FOREACH_LOGLEVEL(_FUNCTION);
#undef _FUNCTION
            };
            return "unknown";
        }
        inline std::ofstream output_file;

        inline void output_log(log_level lev, std::string msg, const std::source_location& loc) {
            msg = std::format("{}:{} [{}] {}", loc.file_name(), loc.line(), _details::log_level_name(lev), msg);
            if (lev >= max_level) {
                std::cout << k_level_ansi_colors[(std::uint8_t)lev] + msg +  k_reset_ansi_color + "\n";
            }

            if (output_file)    
                output_file << msg + "\n";
        }

        template<typename T>
        struct with_source_location {
        private:
            T inner;
            std::source_location loc;

        public:
            template<class U> requires std::constructible_from<T, U>
            consteval with_source_location(U&& inner, std::source_location loc = std::source_location::current()):
            inner(std::forward<U>(inner)), loc(std::move(loc)) {}

            constexpr const T& format() const { return inner; }
            constexpr std::source_location const& location() const { return loc; }
        };
    }

    inline void set_log_level(log_level lev) {
        _details::max_level = lev;
    }

    inline void set_log_file(const std::string& path) {
        _details::output_file = std::ofstream(path, std::ios::app);
    }

    template<typename... Args>
    void log(log_level lev, _details::with_source_location<std::format_string<Args...>> fmt, Args&&... args) {
        auto msg = std::vformat(fmt.format().get(), std::make_format_args(args...));
        auto& loc = fmt.location();
        _details::output_log(lev, msg, loc);
    }

#define _FUNCTION(name) \
template<typename... Args> \
void log_##name(_details::with_source_location<std::format_string<Args...>> fmt, Args&&... args) { \
    log(log_level::name, std::move(fmt), std::forward<Args>(args)...); \
}\

ZSTLLOG_FOREACH_LOGLEVEL(_FUNCTION)
#undef _FUNCTION
}