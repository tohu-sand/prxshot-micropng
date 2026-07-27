/*
 *  micropng - streaming PNG writer for constrained environments
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
 * Writes 8-bit RGB PNGs one row at a time. Self contained: no zlib, no libc
 * allocation, no floating point, and no recursion. All state lives in the
 * caller supplied micropng_t, which is far too large for a kernel thread
 * stack and must be allocated elsewhere.
 */

#ifndef MICROPNG_H
#define MICROPNG_H

#define MICROPNG_MAX_WIDTH  512

/* Deflate window; matches may reach this far back. */
#define MICROPNG_WIN        32768
/* Sliding buffer: one window of history plus one window of lookahead. */
#define MICROPNG_BUF        (MICROPNG_WIN * 2)
#define MICROPNG_HASH_BITS  12
#define MICROPNG_HASH_SIZE  (1 << MICROPNG_HASH_BITS)
/* Payload of one emitted IDAT chunk. */
#define MICROPNG_IDAT       8192

/* Returns the number of bytes written, or a negative value on failure. */
typedef int (*micropng_write_fn)(void *user, const void *data, unsigned int len);

typedef struct {
    micropng_write_fn write;
    void             *user;
    int               failed;

    int width;
    int height;
    int rows_done;

    /* Deflate input window. */
    unsigned char  win[MICROPNG_BUF];
    unsigned int   wpos;          /* bytes appended            */
    unsigned int   pos;           /* bytes already compressed  */
    unsigned int   base;          /* stream offset of win[0]   */
    unsigned int   head[MICROPNG_HASH_SIZE];   /* position + 1; 0 = empty */

    /* Bit packer feeding the IDAT payload buffer. */
    unsigned int  bitbuf;
    int           bitcnt;
    unsigned char idat[MICROPNG_IDAT];
    unsigned int  idat_len;

    unsigned int adler_a;
    unsigned int adler_b;
    unsigned int crc_table[256];

    /* Previous and current unfiltered rows, plus the filtered result. */
    unsigned char prev[MICROPNG_MAX_WIDTH * 3];
    unsigned char cur[MICROPNG_MAX_WIDTH * 3];
    unsigned char filt[MICROPNG_MAX_WIDTH * 3 + 1];
} micropng_t;

/* Emits the signature and IHDR. width must be 1..MICROPNG_MAX_WIDTH. */
int micropng_begin(micropng_t *st, int width, int height,
                   micropng_write_fn write, void *user);

/* Feeds one row of width*3 bytes in R,G,B order. */
int micropng_row(micropng_t *st, const unsigned char *rgb);

/* Flushes the deflate stream and emits IEND. */
int micropng_end(micropng_t *st);

#endif /* MICROPNG_H */
