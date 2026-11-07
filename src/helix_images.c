#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <helix/helix_images.h>
#include <helix/helix_logging.h>
#include <libretro.h>

// Takes uncompressed 24-bit bmp and turns it into raw RGB565
uint16_t* load_bmp_image(const char *file_path, unsigned *out_width, unsigned *out_height, unsigned *out_pitch) {
    if (out_width)  *out_width = 0;
    if (out_height) *out_height = 0;
    if (out_pitch)  *out_pitch = 0;

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        frontend_log_cb(RETRO_LOG_INFO, "HELIX_IMG" ,"Failed to open BMP: %s\n", file_path);
        return NULL;
    }

    BMPFileHeader file_header;
    BMPInfoHeader info_header;

    if (fread(&file_header, sizeof(BMPFileHeader), 1, file) != 1 ||
        fread(&info_header, sizeof(BMPInfoHeader), 1, file) != 1) {
        frontend_log_cb(RETRO_LOG_ERROR, "HELIX_IMG" ,"Failed to read BMP headers.\n");
        fclose(file);
        return NULL;
    }

    if (file_header.type != 0x4D42 || info_header.bits_per_pixel != 24 || info_header.compression != 0) {
        frontend_log_cb(RETRO_LOG_ERROR, "HELIX_IMG" ,"Unsupported BMP format. Must be uncompressed 24-bit.\n");
        fclose(file);
        return NULL;
    }

    unsigned width = info_header.width;
    unsigned height = abs(info_header.height);
    bool flip_vertical = (info_header.height > 0);
    unsigned pitch = width * sizeof(uint16_t);

    uint16_t *rgb565_buffer = (uint16_t *)malloc(width * height * sizeof(uint16_t));
    if (!rgb565_buffer) {
        frontend_log_cb(RETRO_LOG_ERROR, "HELIX_IMG" ,"Memory allocation failed for RGB565 buffer.\n");
        fclose(file);
        return NULL;
    }

    if (fseek(file, file_header.offset, SEEK_SET) != 0) {
        frontend_log_cb(RETRO_LOG_ERROR, "HELIX_IMG" ,"Failed to seek to pixel data.\n");
        free(rgb565_buffer);
        fclose(file);
        return NULL;
    }

    int row_padding = (4 - (width * 3) % 4) % 4;
    uint8_t bgr_pixel[3];

    for (unsigned int y = 0; y < height; y++) {
        int target_y = flip_vertical ? (height - 1 - y) : y;
        uint16_t *row_ptr = rgb565_buffer + (target_y * width);

        for (unsigned int x = 0; x < width; x++) {
            if (fread(bgr_pixel, 3, 1, file) != 1) {
                break;
            }

            uint16_t r = (bgr_pixel[2] & 0xF8) << 8;
            uint16_t g = (bgr_pixel[1] & 0xFC) << 3;
            uint16_t b = (bgr_pixel[0] & 0xF8) >> 3;

            row_ptr[x] = r | g | b;
        }
        fseek(file, row_padding, SEEK_CUR);
    }

    fclose(file);

    if (out_width)  *out_width = width;
    if (out_height) *out_height = height;
    if (out_pitch)  *out_pitch = pitch;

    return rgb565_buffer;
}

// Takes raw RGB565 or RGB32 and turns it into uncompressed 24-bit bmp
bool save_bmp_image(const char *file_path, const void *framebuffer, unsigned width, unsigned height, bool rgb32) {
    // Calculate row padding to a multiple of 4 bytes
    unsigned row_stride = width * 3;
    unsigned row_padding = (4 - (row_stride % 4)) % 4;
    unsigned padded_row_size = row_stride + row_padding;
    unsigned image_data_size = padded_row_size * height;

    unsigned char* img_data = (unsigned char*)malloc(image_data_size);
    if (!img_data) {
        frontend_log_cb(RETRO_LOG_ERROR, "HELIX_IMG", "Memory allocation failed.\n");
        return false;
    }
    memset(img_data, 0, image_data_size);

    for (unsigned y = 0; y < height; y++) {
        unsigned char *row_dest = img_data + (y * padded_row_size);

        for (unsigned x = 0; x < width; x++) {
            unsigned idx = y * width + x;
            uint8_t r, g, b;

            if (rgb32) {
                uint32_t color = ((const uint32_t*)framebuffer)[idx];
                r = (color >> 16) & 0xFF;
                g = (color >> 8) & 0xFF;
                b = color & 0xFF;
            } else {
                uint16_t color = ((const uint16_t*)framebuffer)[idx];
                uint8_t r5 = (color >> 11) & 0x1F;
                uint8_t g6 = (color >> 5) & 0x3F;
                uint8_t b5 = color & 0x1F;

                r = (r5 << 3) | (r5 >> 2);
                g = (g6 << 2) | (g6 >> 4);
                b = (b5 << 3) | (b5 >> 2);
            }
            // BMP stores pixels as BGR
            row_dest[x * 3 + 0] = b;
            row_dest[x * 3 + 1] = g;
            row_dest[x * 3 + 2] = r;
        }
    }

    // BMP Header setup
    BMPFileHeader bmp_header = {0};
    BMPInfoHeader bmp_info_header = {0};

    bmp_header.type = 0x4D42;  // 'BM' in little-endian
    bmp_header.offset = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
    bmp_header.size = bmp_header.offset + image_data_size;

    bmp_info_header.size = sizeof(BMPInfoHeader);
    bmp_info_header.width = width;
    bmp_info_header.height = -height;  // Negative height to indicate top-down BMP
    bmp_info_header.planes = 1;
    bmp_info_header.bits_per_pixel = 24;  // 24 bits per pixel (RGB)
    bmp_info_header.image_size = image_data_size;

    bool ret;
    FILE *bmp_file = fopen(file_path, "wb");
    if (bmp_file) {
        // Write BMP header and info header
        fwrite(&bmp_header, sizeof(BMPFileHeader), 1, bmp_file);
        fwrite(&bmp_info_header, sizeof(BMPInfoHeader), 1, bmp_file);

        fwrite(img_data, 1, image_data_size, bmp_file);

        fflush(bmp_file);
        fsync(fileno(bmp_file));
        fclose(bmp_file);
        frontend_log_cb(RETRO_LOG_INFO, "HELIX_IMG" ,"Screenshot saved to %s\n", file_path);
        ret = true;
    } else {
        frontend_log_cb(RETRO_LOG_ERROR, "HELIX_IMG" ,"Failed to save screenshot\n");
        ret = false;
    }

    free(img_data);
    return ret;
}

// Function to load a raw RGB565 image into the framebuffer
bool load_rgb565_image(const char* file_path, uint16_t* framebuffer, int width, int height) {
    FILE* file = fopen(file_path, "rb");
    if (file == NULL) {
        frontend_log_cb(RETRO_LOG_ERROR, "HELIX_IMG" ,"Error opening file: %s\n", file_path);
        return false;
    }

    size_t image_size = width * height * sizeof(uint16_t);
    size_t bytes_read = fread(framebuffer, 1, image_size, file);
    if (bytes_read != image_size) {
        frontend_log_cb(RETRO_LOG_ERROR, "HELIX_IMG" ,"Error reading RGB565 image file\n");
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}
