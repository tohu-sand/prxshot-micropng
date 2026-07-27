/*
 *  prxshot-png - PNG output
 *
 *  Copyright (C) 2026  tohu_sand
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Converts the display buffer to RGB one row at a time and hands it to
 * micropng. The framebuffer is only ever read.
 */

#include <pspiofilemgr.h>
#include <pspdisplay.h>
#include "png_write.h"
#include "logger.h"

#define SCR_W 480
#define SCR_H 272

typedef struct {
    SceUID fd;
} sink_t;

static int sink_write(void *user, const void *data, unsigned int len)
{
    return sceIoWrite(((sink_t *)user)->fd, data, (SceSize)len);
}

static void row_565(const unsigned short *src, unsigned char *dst)
{
    int x;

    for (x = 0; x < SCR_W; x++) {
        unsigned int c = src[x];
        dst[x * 3 + 0] = (unsigned char)((c & 0x1F) << 3);
        dst[x * 3 + 1] = (unsigned char)(((c >> 5) & 0x3F) << 2);
        dst[x * 3 + 2] = (unsigned char)(((c >> 11) & 0x1F) << 3);
    }
}

static void row_5551(const unsigned short *src, unsigned char *dst)
{
    int x;

    for (x = 0; x < SCR_W; x++) {
        unsigned int c = src[x];
        dst[x * 3 + 0] = (unsigned char)((c & 0x1F) << 3);
        dst[x * 3 + 1] = (unsigned char)(((c >> 5) & 0x1F) << 3);
        dst[x * 3 + 2] = (unsigned char)(((c >> 10) & 0x1F) << 3);
    }
}

static void row_4444(const unsigned short *src, unsigned char *dst)
{
    int x;

    for (x = 0; x < SCR_W; x++) {
        unsigned int c = src[x];
        dst[x * 3 + 0] = (unsigned char)((c & 0x0F) << 4);
        dst[x * 3 + 1] = (unsigned char)(((c >> 4) & 0x0F) << 4);
        dst[x * 3 + 2] = (unsigned char)(((c >> 8) & 0x0F) << 4);
    }
}

static void row_8888(const unsigned int *src, unsigned char *dst)
{
    int x;

    for (x = 0; x < SCR_W; x++) {
        unsigned int c = src[x];
        dst[x * 3 + 0] = (unsigned char)(c & 0xFF);
        dst[x * 3 + 1] = (unsigned char)((c >> 8) & 0xFF);
        dst[x * 3 + 2] = (unsigned char)((c >> 16) & 0xFF);
    }
}

int pngWrite(void *frame_addr, void *scratch, int stride, int format,
             const char *file)
{
    micropng_t    *st   = (micropng_t *)scratch;
    unsigned char *snap = (unsigned char *)scratch + sizeof(micropng_t);
    sink_t         sink;
    int            y, rc = -1;

    if (!frame_addr || !scratch || !file) {
        return -1;
    }
    if (stride <= 0) {
        stride = 512;
    }

    /* Snapshot first: the display buffer keeps being redrawn underneath us,
       and compressing is far slower than converting, so reading row by row
       would tear. One quick pass freezes the frame, then we compress. */
    for (y = 0; y < SCR_H; y++) {
        unsigned char *dst = snap + (unsigned int)y * (SCR_W * 3);

        switch (format) {
        case PSP_DISPLAY_PIXEL_FORMAT_565:
            row_565((const unsigned short *)frame_addr + (unsigned int)(y * stride), dst);
            break;
        case PSP_DISPLAY_PIXEL_FORMAT_5551:
            row_5551((const unsigned short *)frame_addr + (unsigned int)(y * stride), dst);
            break;
        case PSP_DISPLAY_PIXEL_FORMAT_4444:
            row_4444((const unsigned short *)frame_addr + (unsigned int)(y * stride), dst);
            break;
        default:
            row_8888((const unsigned int *)frame_addr + (unsigned int)(y * stride), dst);
            break;
        }
    }

    sink.fd = sceIoOpen(file, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (sink.fd < 0) {
        kprintf("pngWrite: open failed %08X\n", sink.fd);
        return -1;
    }

    if (micropng_begin(st, SCR_W, SCR_H, sink_write, &sink) != 0) {
        goto out;
    }
    for (y = 0; y < SCR_H; y++) {
        if (micropng_row(st, snap + (unsigned int)y * (SCR_W * 3)) != 0) {
            goto out;
        }
    }
    if (micropng_end(st) != 0) {
        goto out;
    }
    rc = 0;

out:
    sceIoClose(sink.fd);
    return rc;
}
