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
    file_header->bfSize    = sizeof(struct BITMAPFILEHEADER)
                           + sizeof(struct BITMAPINFOHEADER)
                           + image_size;
    file_header->bfOffBits = sizeof(struct BITMAPFILEHEADER)
                           + sizeof(struct BITMAPINFOHEADER);
}

static void
create_info_header(struct BITMAPINFOHEADER *info_header, int32_t image_size, int32_t width, int32_t height, int16_t bpp)
{
    info_header->biSize      = sizeof(struct BITMAPINFOHEADER);
    info_header->biWidth     = width;
    info_header->biHeight    = height;
    info_header->biPlanes    = 1;
    info_header->biBitCount  = bpp;
    info_header->biSizeImage = image_size;
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
               int32_t image_size,
               int32_t width,
               int32_t height,
               int16_t bpp)
{
    if ((filename == NULL) || (buffer == NULL)) {
        return -1;
    }

    struct BITMAPFILEHEADER file_header = {};
    struct BITMAPINFOHEADER info_header = {};

    create_file_header(&file_header, image_size);
    create_info_header(&info_header, image_size, width, height, bpp);
    return write_bitmap(filename, &file_header, &info_header, buffer);
}
