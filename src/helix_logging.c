
#include <stdio.h>
#include <stdarg.h>

#include <libretro.h>

void frontend_log_cb(enum retro_log_level level, const char *tag, const char *fmt, ...) {
    char buffer[500];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // Determine the log level string
	// TODO: Add a setting to block logs as a whole and specific log levels
	// Users will probably only want WARN and ERROR logs
    const char *level_str = "";
    switch (level) {
        case RETRO_LOG_DEBUG: level_str = "DEBUG"; break;
        case RETRO_LOG_INFO: level_str = "INFO"; break;
        case RETRO_LOG_WARN: level_str = "WARN"; break;
        case RETRO_LOG_ERROR: level_str = "ERROR"; break;
        default: break;
    }
	// TODO: Right now we just print logs and rely on fileuart
    // While fileuart is useful for debugging, logging ALL print functions might be a bad idea
    // So maybe a new xlog like function would be desirable
    printf("[%s][%s] %s", tag, level_str, buffer);
}

void core_log_cb(enum retro_log_level level, const char *fmt, ...) {
    char buffer[500];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    frontend_log_cb(level, "CORE", "%s", buffer);
}