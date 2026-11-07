#ifndef __HELIX_FAKE_RTC_H
#define __HELIX_FAKE_RTC_H

#include <time.h>
#include <stdbool.h>

time_t fake_rtc_now(void);
bool fake_rtc_init(void);
bool fake_rtc_deinit(void);

#endif //__HELIX_FAKE_RTC_H