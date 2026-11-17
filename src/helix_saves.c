#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

#include <zip.h>
#include <libretro.h>
#include <helix_paths.h>
#include <helix/helix_saves.h>
#include <helix/helix_logging.h>

static void hxstate_get_filename(char *dest, size_t dest_size, const SaveInfo *info, const char *ext) {
    snprintf(dest, dest_size, "[%s][%s][%03d].%s", info->core_name, info->rom_name, info->slot, ext);
}

static void hxsave_get_filename(char *dest, size_t dest_size, const SaveInfo *info, const char *ext, bool per_core_srm) {
    if (per_core_srm) snprintf(dest, dest_size, "%s/[%s]%s.%s", info->save_folder, info->core_name, info->rom_name, ext);
    else snprintf(dest, dest_size, "%s/%s.%s", info->save_folder, info->rom_name, ext);
}

static bool get_save_data(void **out_data, size_t *out_size, unsigned int id) {
    *out_size = retro_get_memory_size(id);
    if(*out_size == 0) return false;
    *out_data = retro_get_memory_data(id);
    if (*out_data  == NULL) return false;
    return true;
}

static bool load_save_data(const char *filepath, unsigned int id) {
    void *ram_data = retro_get_memory_data(id);
    if (!ram_data) return false;
    
    FILE *ram_file = fopen(filepath, "rb");
	if (!ram_file) return false;
    
	fseeko(ram_file, 0, SEEK_END);
	long ram_file_size = ftell(ram_file);
	fseeko(ram_file, 0, SEEK_SET);
    if (ram_file_size < 0) goto fail;

    size_t ram_size = retro_get_memory_size(id);
	if((size_t)ram_file_size < ram_size) ram_size = (size_t)ram_file_size;
	if(ram_size == 0) goto fail;

	fread(ram_data, 1, ram_size, ram_file);
	fclose(ram_file);
    return true;

    fail:
        if (ram_file) fclose(ram_file);
        return false;
}

void helix_save_srm(SaveInfo saveinfo, bool per_core_srm) {
    char ram_filepath[MAXPATH];
    size_t ram_size = 0;
    void *ram_data = NULL;

    // Save SRM
    hxsave_get_filename(ram_filepath, sizeof(ram_filepath), &saveinfo, "srm", per_core_srm);
	if (get_save_data(&ram_data, &ram_size, RETRO_MEMORY_SAVE_RAM)) {
        FILE *srm_file = fopen(ram_filepath, "wb");
        if (srm_file) {
            fwrite(ram_data, ram_size, 1, srm_file);
            fsync(fileno(srm_file));
	        fclose(srm_file);
        }
    }

    // Save RTC
    hxsave_get_filename(ram_filepath, sizeof(ram_filepath), &saveinfo, "rtc", per_core_srm);
	if (get_save_data(&ram_data, &ram_size, RETRO_MEMORY_RTC)) {
        FILE *rtc_file = fopen(ram_filepath, "wb");
        if (rtc_file) {
            fwrite(ram_data, ram_size, 1, rtc_file);
            fsync(fileno(rtc_file));
	        fclose(rtc_file);
        }
    }
}

void helix_load_srm(SaveInfo saveinfo, bool per_core_srm) {
    char ram_filepath[MAXPATH];

    // Load SRM
    hxsave_get_filename(ram_filepath, sizeof(ram_filepath), &saveinfo, "srm", per_core_srm);
    load_save_data(ram_filepath, RETRO_MEMORY_SAVE_RAM);

	// Load RTC
    hxsave_get_filename(ram_filepath, sizeof(ram_filepath), &saveinfo, "rtc", per_core_srm);
    load_save_data(ram_filepath, RETRO_MEMORY_RTC);
}

bool helix_state_save(SaveInfo saveinfo) {
    char temp_filename[MAXPATH];
    void *temp_data = NULL;
    size_t temp_size = 0;
    void *out_buffer = NULL;
    size_t out_size = 0;
    struct zip_t *zip = NULL;
    bool ret = false;

    char hxstate_filepath[MAXPATH];
    snprintf(hxstate_filepath, sizeof(hxstate_filepath), "%s/[%s][%s][%03d].hxstate", saveinfo.savestate_folder, saveinfo.core_name, saveinfo.rom_name, saveinfo.slot);

	temp_size = retro_serialize_size();
    if (temp_size == 0) goto cleanup;
	temp_data = calloc(temp_size, 1);
    if (!temp_data) goto log;

	retro_serialize(temp_data, temp_size);

    zip = zip_stream_open(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
    if (!zip) goto log;

    // State file
    hxstate_get_filename(temp_filename, sizeof(temp_filename), &saveinfo, "state");
    zip_entry_open(zip, temp_filename);
    zip_entry_write(zip, temp_data, temp_size);
    zip_entry_close(zip);

	if (temp_data) {
        free(temp_data);
        temp_data = NULL;
    }

    // SRM file
    if (get_save_data(&temp_data, &temp_size, RETRO_MEMORY_SAVE_RAM)) {
        hxstate_get_filename(temp_filename, sizeof(temp_filename), &saveinfo, "srm");
        zip_entry_open(zip, temp_filename);
        zip_entry_write(zip, temp_data, temp_size);
        zip_entry_close(zip);
    }

    // RTC file
	if (get_save_data(&temp_data, &temp_size, RETRO_MEMORY_RTC)) {
        hxstate_get_filename(temp_filename, sizeof(temp_filename), &saveinfo, "rtc");
        zip_entry_open(zip, temp_filename);
        zip_entry_write(zip, temp_data, temp_size);
        zip_entry_close(zip);
    }

    // TODO: Add BMP file

    ssize_t written = zip_stream_copy(zip, &out_buffer, &out_size);
    if (written < 0) goto log;

    FILE *f = fopen(hxstate_filepath, "wb");
    if (f) {
        fwrite(out_buffer, 1, out_size, f);
        fflush(f);
        fsync(fileno(f));
        fclose(f);
        ret = true;
    }

    log:
        if (ret) frontend_log_cb(RETRO_LOG_INFO, "HELIX_STATE" ,"state_save: %s\n", hxstate_filepath);
        else frontend_log_cb(RETRO_LOG_ERROR, "HELIX_STATE" ,"Failed to save savestate: %s\n", hxstate_filepath);
    cleanup:
        if (zip) zip_close(zip);
        if (out_buffer) free(out_buffer);
        return ret;
}

bool helix_state_load(SaveInfo saveinfo, bool dont_overwrite_saveram) {
    char temp_filename[MAXPATH];
    void *buf = NULL;
    size_t bufsize = 0;
    bool ret = false;

    void *srm_backup = NULL;
    void *rtc_backup = NULL;
    size_t srm_size = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    size_t rtc_size = retro_get_memory_size(RETRO_MEMORY_RTC);

    char hxstate_filepath[MAXPATH];
    snprintf(hxstate_filepath, sizeof(hxstate_filepath), "%s/[%s][%s][%03d].hxstate", saveinfo.savestate_folder, saveinfo.core_name, saveinfo.rom_name, saveinfo.slot);

    struct zip_t *zip = zip_open(hxstate_filepath, 0, 'r');
	if (!zip) goto cleanup;

    hxstate_get_filename(temp_filename, sizeof(temp_filename), &saveinfo, "state");
    if (zip_entry_open(zip, temp_filename) != 0) goto log;

    if (dont_overwrite_saveram) {
        if (srm_size > 0) {
            void *current_srm = retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
            if (current_srm) {
                srm_backup = malloc(srm_size);
                if (srm_backup) memcpy(srm_backup, current_srm, srm_size);
            }
        }
        if (rtc_size > 0) {
            void *current_rtc = retro_get_memory_data(RETRO_MEMORY_RTC);
            if (current_rtc) {
                rtc_backup = malloc(rtc_size);
                if (rtc_backup) memcpy(rtc_backup, current_rtc, rtc_size);
            }
        }
    }

    zip_entry_read(zip, &buf, &bufsize);
    zip_entry_close(zip);
	if (!retro_unserialize(buf, bufsize)) goto log;

    if (dont_overwrite_saveram) {
        if (srm_backup && srm_size > 0) {
            void *current_srm = retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
            if (current_srm) memcpy(current_srm, srm_backup, srm_size);
        }
        if (rtc_backup && rtc_size > 0) {
            void *current_rtc = retro_get_memory_data(RETRO_MEMORY_RTC);
            if (current_rtc) memcpy(current_rtc, rtc_backup, rtc_size);
        }
    }
	
	ret = true;
    log:
        if (ret) frontend_log_cb(RETRO_LOG_INFO, "HELIX_STATE" ,"state_load: %s\n", hxstate_filepath);
        else frontend_log_cb(RETRO_LOG_ERROR, "HELIX_STATE" ,"Failed to load savestate: %s\n", hxstate_filepath);
    cleanup:
        if (buf) free(buf);
        if (srm_backup) free(srm_backup);
        if (rtc_backup) free(rtc_backup);
        if (zip) zip_close(zip);
        return ret;
}