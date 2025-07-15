#ifndef LOGS_H
#define LOGS_H

namespace logging {

// Available log levels.
struct Level { enum E { Info = 'I' , Warning = 'W', Error = 'E' }; };

// Initialise the logging subsystem.
void init(const char *path);

// Print a message at a given log level.
void print(Level::E level, const char *msg, ...) __attribute__((format(printf, 2, 3)));

} // namespace logging

#endif
