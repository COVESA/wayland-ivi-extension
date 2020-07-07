/***************************************************************************
 *
 * Copyright (C) 2020 Advanced Driver Information Technology Joint Venture GmbH
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "png.h"
#include "ivi-wm-client-protocol.h"
#include "writepng.h"

typedef struct _image_info {
    png_structp  png_ptr;
    png_infop    info_ptr;
    jmp_buf      jmpbuf;
    FILE        *outfile;
} image_info;


static void
writepng_error_handler(png_structp png_ptr,
                       png_const_charp msg)
{
    fprintf(stderr, "writepng libpng error: %s\n", msg);
}

static int
create_png_header(image_info *info,
                  int32_t width,
                  int32_t height,
                  uint32_t format)
{
    int color_type = 0;
    int sample_depth = 8;

    info->info_ptr = png_create_info_struct(info->png_ptr);
    if (!info->info_ptr) {
        fprintf(stderr, "png_create_info_struct: failed, out of memory\n");
        png_destroy_write_struct(&info->png_ptr, NULL);
        return -1;
    }

    if (setjmp(info->jmpbuf)) {
        fprintf(stderr, "setjmp: failed\n");
        png_destroy_write_struct(&info->png_ptr, &info->info_ptr);
        return -1;
    }

    png_init_io(info->png_ptr, info->outfile);

    png_set_compression_level(info->png_ptr, PNG_Z_DEFAULT_COMPRESSION);

    switch (format) {
    case WL_SHM_FORMAT_ARGB8888:
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        png_set_swap_alpha(info->png_ptr);
        break;
    case WL_SHM_FORMAT_XRGB8888:
        color_type = PNG_COLOR_TYPE_RGB;
        break;
    case WL_SHM_FORMAT_ABGR8888:
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        png_set_bgr(info->png_ptr);
        png_set_swap_alpha(info->png_ptr);
        break;
    case WL_SHM_FORMAT_XBGR8888:
        color_type = PNG_COLOR_TYPE_RGB;
        png_set_bgr(info->png_ptr);
        break;
    default:
        fprintf(stderr, "unsupported pixelformat 0x%x\n", format);
        return -1;
    }

    png_set_IHDR(info->png_ptr, info->info_ptr, width, height,
                 sample_depth, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(info->png_ptr, info->info_ptr);

    return 0;
}

static int
write_png_file(image_info *info,
               const char *buffer,
               int32_t width,
               int32_t height,
               uint32_t format,
               int bytes_per_pixel)
{
    info->png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
                                      writepng_error_handler, NULL);
    if (!info->png_ptr) {
        fprintf(stderr, "png_create_write_struct: failed, out of memory\n");
        return -1;
    }

    if (create_png_header(info, width, height, format) != 0) {
        return -1;
    }

    for(int j = 0; j < height  ; ++j) {
        png_const_bytep pointer = (png_const_bytep)buffer;
        pointer += j * width * bytes_per_pixel;

        if (setjmp(info->jmpbuf)) {
            fprintf(stderr, "setjmp: failed, j=%d\n", j);
            png_destroy_write_struct(&info->png_ptr, &info->info_ptr);
            info->png_ptr = NULL;
            info->info_ptr = NULL;
            return -1;
        }

        png_write_row(info->png_ptr, pointer);
    }

    if (setjmp(info->jmpbuf)) {
        fprintf(stderr, "final setjmp failed\n");
        png_destroy_write_struct(&info->png_ptr, &info->info_ptr);
        info->png_ptr = NULL;
        info->info_ptr = NULL;
        return -1;
    }

    png_write_end(info->png_ptr, NULL);

    if (info->png_ptr && info->info_ptr) {
        png_destroy_write_struct(&info->png_ptr, &info->info_ptr);
    }

    return 0;
}

int
save_as_png(const char *filename,
            const char *buffer,
            int32_t width,
            int32_t height,
            uint32_t format)
{
    int32_t image_stride = 0;
    int32_t image_size = 0;
    char *image_buffer = NULL;
    int32_t row = 0;
    int32_t col = 0;
    int32_t image_offset = 0;
    int32_t offset = 0;
    int bytes_per_pixel = 0;
    bool has_alpha = false;
    image_info info;

    if ((filename == NULL) || (buffer == NULL)) {
        return -1;
    }

    info.outfile = fopen(filename, "wb");
    if (!info.outfile) {
        fprintf(stderr, "could not open the file %s\n", filename);
        return -1;
    }

    switch (format) {
    case WL_SHM_FORMAT_ARGB8888:
    case WL_SHM_FORMAT_ABGR8888:
        has_alpha = true;
        break;
    default:
        has_alpha = false;
        break;
    }

    bytes_per_pixel = has_alpha ? 4 : 3;
    image_stride = (((width * bytes_per_pixel) + 3) & ~3);
    image_size = image_stride * height;

    image_buffer = malloc(image_size);
    if (image_buffer == NULL) {
        fprintf(stderr, "failed to allocate %d bytes for image buffer: %m\n",
                image_size);
        return -1;
    }

    for (row = 0; row < height; ++row) {
        for (col = 0; col < width; ++col) {
            offset = row * width + col;
            uint32_t pixel = htonl(((uint32_t*)buffer)[offset]);
            char * pixel_p = (char*) &pixel;
            image_offset = row * image_stride + col * bytes_per_pixel;
            for (int i=0; i<bytes_per_pixel; ++i){
                image_buffer[image_offset + i] = pixel_p[i + (has_alpha ? 0 : 1)];
            }
        }
    }

    if (write_png_file(&info, image_buffer, width, height, format, bytes_per_pixel) != 0) {
        free(image_buffer);
        return -1;
    }

    free(image_buffer);
    return 0;
}
