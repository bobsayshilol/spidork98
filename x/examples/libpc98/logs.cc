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
    va_start(args, msg);

    const unsigned long long t_ms = Funcs98::ticks_per_sec() / 1000;
    fprintf(s_log_file, "[%c][%u] ", static_cast<char>(level), static_cast<unsigned>(Funcs98::ticks() / t_ms));
    vfprintf(s_log_file, msg, args);
    fprintf(s_log_file, "\n");

    va_end(args);
}

} // namespace logging
