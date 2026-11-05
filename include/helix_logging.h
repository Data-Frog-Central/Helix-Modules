#ifndef __HELIX_LOGGING_H
#define __HELIX_LOGGING_H

#include <libretro.h>

void frontend_log_cb(enum retro_log_level level, const char *tag, const char *fmt, ...);
void core_log_cb(enum retro_log_level level, const char *fmt, ...);

#endif //__HELIX_LOGGING_H