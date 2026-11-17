#ifndef __HELIX_SAVES_H
#define __HELIX_SAVES_H

typedef struct {
    char save_folder[MAXPATH];
    char savestate_folder[MAXPATH];
    char core_name[MAXPATH];
    char rom_name[MAXPATH];
    int slot;
} SaveInfo;

typedef struct {
    uint16_t emu_fb;
    uint32_t fb_width;
    uint32_t fb_height;
    size_t fb_size;
} ImageInfo;

void helix_save_srm(SaveInfo saveinfo, bool per_core_srm);
void helix_load_srm(SaveInfo saveinfo, bool per_core_srm);
bool helix_state_save(SaveInfo saveinfo);
bool helix_state_load(SaveInfo saveinfo, bool dont_overwrite_saveram);

#endif //__HELIX_SAVES_H