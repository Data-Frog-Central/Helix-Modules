#ifndef __HELIX_LOGGING_H
#define __HELIX_LOGGING_H

#include <libretro.h>

void set_min_log_level(int level);
void frontend_log_cb(enum retro_log_level level, const char *tag, const char *fmt, ...);
void core_log_cb(enum retro_log_level level, const char *fmt, ...);

void clear_xlog(const char *filename);
void init_xlog(const char *filename, bool clear_logs, bool fileuart);
void sync_xlog(void);
void close_xlog(void);
void xlog(const char *fmt, ...);

#endif //__HELIX_LOGGING_H
