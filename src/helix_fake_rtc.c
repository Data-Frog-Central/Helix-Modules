#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

#include <helix_paths.h>
#include <helix/helix_logging.h>

static time_t base_time;
static time_t start_time;

static time_t os_get_time(void) {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return ts.tv_sec;
    }

    return (time_t)-1;
}

// Load the RTC from a file
static bool load_rtc_file(time_t *base_time) {
    FILE *file = fopen(RTC_PATH, "r");
    if (!file) return false;

    // Read the RTC time as a string from the file
    if (fscanf(file, "%lld", base_time) != 1) {
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

// Save the RTC to a file
static bool save_rtc_file(time_t base_time) {
    FILE *file = fopen(RTC_PATH, "w");
    if (!file) {
        frontend_log_cb(RETRO_LOG_INFO, "FAKE_RTC" ,"Failed opening RTC file for writing: %s\n", RTC_PATH);
        return false;
    }

    // Write the RTC time as a string to the file
    fprintf(file, "%lld\n", base_time);
    fflush(file);
    fsync(fileno(file));
    fclose(file);
    return true;
}

time_t fake_rtc_now(void) {
    time_t delta_time = os_get_time() - start_time;  
    return base_time + delta_time;
}

bool fake_rtc_init(void) {
    bool ret = true;
    if (!load_rtc_file(&base_time)) {
        // default epoch: 2000-01-01
        base_time = 946684800;
        ret = false;
    }

    start_time = os_get_time();
    return ret;
}

bool fake_rtc_deinit(void) {
    base_time = fake_rtc_now();
    return save_rtc_file(base_time);
}