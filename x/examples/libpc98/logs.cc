#include "logs.h"
#include "funcs.h"
#include "macros.h"

#include <cstdarg>
#include <cstdio>

namespace logging {

namespace {

FILE *s_log_file;

void close_log() {
    if (s_log_file && s_log_file != stdout) {
        fclose(s_log_file);
        s_log_file = NULL;
    }
}

DEFER(void*, p, NULL, close_log());

} // namespace

void init(const char *log_path) {
    if (s_log_file) return;

    if (log_path) {
        s_log_file = fopen(log_path, "wb");
        if (!s_log_file) {
            printf("Failed to open log file: %s\n", log_path);
        }
    }

    if (!s_log_file) {
        s_log_file = stdout;
    }

    print(Level::Info, "Log started");
}

void print(Level::E level, const char *msg, ...) {
    if (!s_log_file) {
        init(NULL);
    }

    va_list args;
    const unsigned t_ms = static_cast<unsigned>(Funcs98::ticks() / (Funcs98::ticks_per_sec() / 1000));

    fprintf(s_log_file, "[%c][%u.%04u] ", static_cast<char>(level), t_ms / 1000, t_ms % 1000);
    va_start(args, msg);
    vfprintf(s_log_file, msg, args);
    va_end(args);
    fprintf(s_log_file, "\n");

#ifdef WEB_BUILD
    printf("[%c][%u.%04u] ", static_cast<char>(level), t_ms / 1000, t_ms % 1000);
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
    printf("\n");
#endif
}

} // namespace logging
