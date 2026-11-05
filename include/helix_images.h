#ifndef __HELIX_IMAGES_H
#define __HELIX_IMAGES_H

#include <stdint.h>
#include <stdbool.h>

// BMP Header Structures
#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BMPFileHeader;

typedef struct {
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t  x_pixels_per_m;
    int32_t  y_pixels_per_m;
    uint32_t colors_used;
    uint32_t colors_important;
} BMPInfoHeader;
#pragma pack(pop)

uint16_t* load_bmp_image(const char *file_path, unsigned *out_width, unsigned *out_height, unsigned *out_pitch);
bool save_bmp_image(const char *file_path, const void *framebuffer, unsigned width, unsigned height, bool rgb32);
bool load_rgb565_image(const char* file_path, uint16_t* framebuffer, int width, int height);

#endif //__HELIX_IMAGES_H