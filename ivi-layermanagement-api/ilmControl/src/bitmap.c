/*
 * Copyright (C) 2013 DENSO CORPORATION
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "ivi-wm-client-protocol.h"

struct __attribute__ ((__packed__)) BITMAPFILEHEADER {
    char bfType[2];
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct __attribute__ ((__packed__)) BITMAPINFOHEADER {
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXPixPerMeter;
    uint32_t biYPixPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImporant;
};

static void
create_file_header(struct BITMAPFILEHEADER *file_header, int32_t image_size)
{
    file_header->bfType[0] = 'B';
    file_header->bfType[1] = 'M';
    file_header->bfSize    = htole32(sizeof(struct BITMAPFILEHEADER) +
                                     sizeof(struct BITMAPINFOHEADER) +
                                     image_size);
    file_header->bfOffBits = htole32(sizeof(struct BITMAPFILEHEADER) +
                                     sizeof(struct BITMAPINFOHEADER));
}

static void
create_info_header(struct BITMAPINFOHEADER *info_header, int32_t image_size, int32_t width, int32_t height, int16_t bpp)
{
    info_header->biSize      = htole32(sizeof(struct BITMAPINFOHEADER));
    info_header->biWidth     = htole32(width);
    info_header->biHeight    = htole32(height);
    info_header->biPlanes    = htole16(1);
    info_header->biBitCount  = htole16(bpp);
    info_header->biSizeImage = htole32(image_size);
}

static int
write_bitmap(const char *filename,
             const struct BITMAPFILEHEADER *file_header,
             const struct BITMAPINFOHEADER *info_header,
             const char *buffer)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        return -1;
    }

    fwrite(file_header, sizeof(struct BITMAPFILEHEADER), 1, fp);
    fwrite(info_header, sizeof(struct BITMAPINFOHEADER), 1, fp);
    fwrite(buffer, info_header->biSizeImage, 1, fp);

    fclose(fp);
    return 0;
}

int
save_as_bitmap(const char *filename,
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
    int32_t i = 0;
    int32_t j = 0;
    int bytes_per_pixel;
    bool flip_order;
    bool has_alpha;

    if ((filename == NULL) || (buffer == NULL)) {
        return -1;
    }

    switch (format) {
    case WL_SHM_FORMAT_ARGB8888:
        flip_order = true;
        has_alpha = true;
        break;
    case WL_SHM_FORMAT_XRGB8888:
        flip_order = true;
        has_alpha = false;
        break;
    case WL_SHM_FORMAT_ABGR8888:
        flip_order = false;
        has_alpha = true;
        break;
    case WL_SHM_FORMAT_XBGR8888:
        flip_order = false;
        has_alpha = false;
        break;
    default:
        fprintf(stderr, "unsupported pixelformat 0x%x\n", format);
        return -1;
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

    // Store the image in image_buffer in the follwing order B, G, R, [A](B at the lowest address)
    for (row = 0; row < height; ++row) {
        for (col = 0; col < width; ++col) {
            offset = (height - row - 1) * width + col;
            uint32_t pixel = htonl(((uint32_t*)buffer)[offset]);
            char * pixel_p = (char*) &pixel;
            image_offset = row * image_stride + col * bytes_per_pixel;
            for (i = 0; i < 3; ++i) {
                j = flip_order ? 2 - i : i;
                image_buffer[image_offset + i] = pixel_p[1 + j];
            }
            if (has_alpha) {
                image_buffer[image_offset + 3] = pixel_p[0];
            }
        }
    }

    struct BITMAPFILEHEADER file_header = {};
    struct BITMAPINFOHEADER info_header = {};

    create_file_header(&file_header, image_size);
    create_info_header(&info_header, image_size, width, height, bytes_per_pixel * 8);
    if (write_bitmap(filename, &file_header, &info_header, image_buffer) != 0) {
        free(image_buffer);
        return -1;
    }

    free(image_buffer);
    return 0;
}
