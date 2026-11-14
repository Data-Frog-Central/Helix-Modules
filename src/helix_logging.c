
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdbool.h>

#include <libretro.h>

// 0 = DEBUG, 1 = INFO, 2 = WARN, 3 = ERROR, 4 = DISABLED
static int min_log_level = 2; // Default to WARN
static FILE *log_file = NULL;
static bool fileuart_enabled = false;

void set_min_log_level(int level) {
    if (level < 0 || level > 4) return;
    min_log_level = level;
}

void clear_xlog(const char *filename) {
	FILE *log = fopen(filename, "w");
	if (!log) return;

	fflush(log);
    fsync(fileno(log));
    fclose(log);
}

void init_xlog(const char *filename, bool clear_logs, bool fileuart) {
    if (log_file) return;
    fileuart_enabled = fileuart;

    if (access(filename, F_OK) == -1) return;
    if (clear_logs) clear_xlog(filename);

    log_file = fopen(filename, "a");
    if (log_file) {
        static char log_buffer[1024];
        setvbuf(log_file, log_buffer, _IOFBF, sizeof(log_buffer));
    }
}

void sync_xlog(void) {
    if (!log_file) return;

    fflush(log_file);
    fsync(fileno(log_file));
}

void close_xlog(void) {
    if (!log_file) return;

    sync_xlog();
    fclose(log_file);
    log_file = NULL;
}

void xlog(const char *fmt, ...) {
    if (!log_file && !fileuart_enabled) return;

    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len <= 0) return;

    if (!log_file && fileuart_enabled) {
        printf("%s", buffer);
        return;
    }

    size_t write_len = ((size_t)len < sizeof(buffer)) ? (size_t)len : sizeof(buffer) - 1;
    fwrite(buffer, 1, write_len, log_file);
}

void frontend_log_cb(enum retro_log_level level, const char *tag, const char *fmt, ...) {
    if ((int)level < 0 || (int)level < min_log_level || (int)level > 3) return;
    char buffer[500];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // Determine the log level string
    const char *level_str = "";
    switch (level) {
        case RETRO_LOG_DEBUG: level_str = "DEBUG"; break;
        case RETRO_LOG_INFO: level_str = "INFO"; break;
        case RETRO_LOG_WARN: level_str = "WARN"; break;
        case RETRO_LOG_ERROR: level_str = "ERROR"; break;
        default: break;
    }

    xlog("[%s][%s] %s", tag, level_str, buffer);
    if (level >= RETRO_LOG_WARN) sync_xlog(); // Immediately log warns/errors
}

void core_log_cb(enum retro_log_level level, const char *fmt, ...) {
    if ((int)level < 0 || (int)level < min_log_level || (int)level > 3) return;
    char buffer[500];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    frontend_log_cb(level, "CORE", "%s", buffer);
}
