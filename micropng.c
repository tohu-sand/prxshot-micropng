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

#include <string.h>
#include "micropng.h"

#define MIN_MATCH  3
#define MAX_MATCH  258
/* Below this the tail of the window may still grow, so matches are deferred. */
#define LOOKAHEAD  (MAX_MATCH + MIN_MATCH)
#define BPP        3

/* ------------------------------------------------------------------ */
/* Output                                                              */
/* ------------------------------------------------------------------ */

static void emit(micropng_t *st, const void *data, unsigned int len)
{
    if (st->failed) {
        return;
    }
    if (st->write(st->user, data, len) < 0) {
        st->failed = 1;
    }
}

static void be32(unsigned char *p, unsigned int v)
{
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)v;
}

static void crc_init(micropng_t *st)
{
    unsigned int n, k, c;

    for (n = 0; n < 256; n++) {
        c = n;
        for (k = 0; k < 8; k++) {
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        st->crc_table[n] = c;
    }
}

static unsigned int crc_add(const micropng_t *st, unsigned int crc,
                            const unsigned char *p, unsigned int len)
{
    unsigned int i;

    for (i = 0; i < len; i++) {
        crc = st->crc_table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc;
}

/* One complete chunk: length, type, payload, CRC of type+payload. */
static void chunk(micropng_t *st, const char *type,
                  const unsigned char *data, unsigned int len)
{
    unsigned char hdr[8];
    unsigned char crcbuf[4];
    unsigned int  crc;

    be32(hdr, len);
    memcpy(hdr + 4, type, 4);
    emit(st, hdr, 8);
    if (len) {
        emit(st, data, len);
    }
    crc = crc_add(st, 0xFFFFFFFFu, hdr + 4, 4);
    crc = crc_add(st, crc, data, len);
    be32(crcbuf, crc ^ 0xFFFFFFFFu);
    emit(st, crcbuf, 4);
}

/* The compressed stream is split across as many IDATs as it needs. */
static void idat_flush(micropng_t *st)
{
    if (st->idat_len) {
        chunk(st, "IDAT", st->idat, st->idat_len);
        st->idat_len = 0;
    }
}

static void idat_byte(micropng_t *st, unsigned char b)
{
    st->idat[st->idat_len++] = b;
    if (st->idat_len == MICROPNG_IDAT) {
        idat_flush(st);
    }
}

/* ------------------------------------------------------------------ */
/* Bit packing (deflate order: LSB first, Huffman codes MSB first)     */
/* ------------------------------------------------------------------ */

static void put_bits(micropng_t *st, unsigned int code, int n)
{
    st->bitbuf |= (code & ((1u << n) - 1u)) << st->bitcnt;
    st->bitcnt += n;
    while (st->bitcnt >= 8) {
        idat_byte(st, (unsigned char)(st->bitbuf & 0xFF));
        st->bitbuf >>= 8;
        st->bitcnt -= 8;
    }
}

static void put_huff(micropng_t *st, unsigned int code, int n)
{
    unsigned int rev = 0;
    int i;

    for (i = 0; i < n; i++) {
        rev = (rev << 1) | ((code >> i) & 1u);
    }
    put_bits(st, rev, n);
}

static void put_literal(micropng_t *st, unsigned int lit)
{
    if (lit < 144) {
        put_huff(st, 0x30 + lit, 8);
    } else {
        put_huff(st, 0x190 + (lit - 144), 9);
    }
}

static void put_symbol(micropng_t *st, unsigned int sym)
{
    if (sym < 144) {
        put_huff(st, 0x30 + sym, 8);
    } else if (sym < 256) {
        put_huff(st, 0x190 + (sym - 144), 9);
    } else if (sym < 280) {
        put_huff(st, sym - 256, 7);
    } else {
        put_huff(st, 0xC0 + (sym - 280), 8);
    }
}

static const unsigned short len_base[29] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};
static const unsigned char len_extra[29] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};
static const unsigned short dist_base[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
    8193, 12289, 16385, 24577
};
static const unsigned char dist_extra[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

static void put_match(micropng_t *st, unsigned int len, unsigned int dist)
{
    int i;

    for (i = 28; i > 0; i--) {
        if (len >= len_base[i]) {
            break;
        }
    }
    put_symbol(st, 257 + (unsigned int)i);
    if (len_extra[i]) {
        put_bits(st, len - len_base[i], len_extra[i]);
    }

    for (i = 29; i > 0; i--) {
        if (dist >= dist_base[i]) {
            break;
        }
    }
    put_huff(st, (unsigned int)i, 5);
    if (dist_extra[i]) {
        put_bits(st, dist - dist_base[i], dist_extra[i]);
    }
}

/* ------------------------------------------------------------------ */
/* LZ77 over the sliding window                                        */
/* ------------------------------------------------------------------ */

static unsigned int hash3(const unsigned char *p)
{
    return (((unsigned int)p[0] << 10) ^ ((unsigned int)p[1] << 5) ^
            (unsigned int)p[2]) & (MICROPNG_HASH_SIZE - 1);
}

static void insert_hash(micropng_t *st, unsigned int p)
{
    if (p + MIN_MATCH <= st->wpos) {
        st->head[hash3(st->win + p)] = p + 1;
    }
}

/* Compresses buffered input. Unless flush is set it keeps LOOKAHEAD bytes
   back so a match can still be extended by the next row. */
static void compress_window(micropng_t *st, int flush)
{
    unsigned int limit;

    for (;;) {
        unsigned int avail = st->wpos - st->pos;
        unsigned int cand, cp, dist, len, maxlen;

        if (!flush && avail < LOOKAHEAD) {
            return;
        }
        if (avail == 0) {
            return;
        }
        if (avail < MIN_MATCH) {
            put_literal(st, st->win[st->pos]);
            st->pos++;
            continue;
        }

        cand = st->head[hash3(st->win + st->pos)];
        st->head[hash3(st->win + st->pos)] = st->pos + 1;

        len = 0;
        if (cand) {
            cp = cand - 1;
            if (cp < st->pos) {
                dist = st->pos - cp;
                if (dist <= MICROPNG_WIN) {
                    maxlen = avail < MAX_MATCH ? avail : MAX_MATCH;
                    while (len < maxlen &&
                           st->win[cp + len] == st->win[st->pos + len]) {
                        len++;
                    }
                }
            }
        }

        if (len >= MIN_MATCH) {
            dist = st->pos - (cand - 1);
            put_match(st, len, dist);
            limit = st->pos + len;
            st->pos++;
            while (st->pos < limit) {
                insert_hash(st, st->pos);
                st->pos++;
            }
        } else {
            put_literal(st, st->win[st->pos]);
            st->pos++;
        }
    }
}

/* Frees the front half of the buffer once the lookahead no longer fits. */
static void slide(micropng_t *st)
{
    unsigned int i;

    if (st->wpos < MICROPNG_BUF - (MICROPNG_MAX_WIDTH * 3 + 1)) {
        return;
    }
    memmove(st->win, st->win + MICROPNG_WIN, st->wpos - MICROPNG_WIN);
    st->wpos -= MICROPNG_WIN;
    st->pos  -= MICROPNG_WIN;
    st->base += MICROPNG_WIN;
    for (i = 0; i < MICROPNG_HASH_SIZE; i++) {
        st->head[i] = (st->head[i] > MICROPNG_WIN) ?
                      (st->head[i] - MICROPNG_WIN) : 0;
    }
}

static void feed(micropng_t *st, const unsigned char *p, unsigned int len)
{
    unsigned int i;

    for (i = 0; i < len; i++) {
        st->adler_a += p[i];
        if (st->adler_a >= 65521u) {
            st->adler_a -= 65521u;
        }
        st->adler_b += st->adler_a;
        if (st->adler_b >= 65521u) {
            st->adler_b -= 65521u;
        }
    }
    memcpy(st->win + st->wpos, p, len);
    st->wpos += len;
    compress_window(st, 0);
    slide(st);
}

/* ------------------------------------------------------------------ */
/* Row filtering                                                       */
/* ------------------------------------------------------------------ */

static int paeth(int a, int b, int c)
{
    int p  = a + b - c;
    int pa = p > a ? p - a : a - p;
    int pb = p > b ? p - b : b - p;
    int pc = p > c ? p - c : c - p;

    if (pa <= pb && pa <= pc) {
        return a;
    }
    return (pb <= pc) ? b : c;
}

/* Sum of absolute differences, treating bytes as signed: the standard
   heuristic for guessing which filter will compress best. */
static unsigned int score(const unsigned char *p, int n)
{
    unsigned int s = 0;
    int i, v;

    for (i = 0; i < n; i++) {
        v = (signed char)p[i];
        s += (unsigned int)(v < 0 ? -v : v);
    }
    return s;
}

static void filter_row(micropng_t *st)
{
    int n = st->width * BPP;
    int i, best = 0;
    unsigned int bs, s;
    unsigned char *out = st->filt + 1;

    /* None */
    bs = score(st->cur, n);

    /* Sub */
    for (i = 0; i < n; i++) {
        out[i] = (unsigned char)(st->cur[i] - (i >= BPP ? st->cur[i - BPP] : 0));
    }
    s = score(out, n);
    if (s < bs) {
        bs = s;
        best = 1;
    }

    /* Up */
    for (i = 0; i < n; i++) {
        out[i] = (unsigned char)(st->cur[i] - st->prev[i]);
    }
    s = score(out, n);
    if (s < bs) {
        bs = s;
        best = 2;
    }

    /* Paeth */
    for (i = 0; i < n; i++) {
        int a = i >= BPP ? st->cur[i - BPP] : 0;
        int b = st->prev[i];
        int c = i >= BPP ? st->prev[i - BPP] : 0;
        out[i] = (unsigned char)(st->cur[i] - paeth(a, b, c));
    }
    s = score(out, n);
    if (s < bs) {
        bs = s;
        best = 4;
    }

    st->filt[0] = (unsigned char)best;
    switch (best) {
    case 0:
        memcpy(out, st->cur, (unsigned int)n);
        break;
    case 1:
        for (i = 0; i < n; i++) {
            out[i] = (unsigned char)(st->cur[i] -
                                     (i >= BPP ? st->cur[i - BPP] : 0));
        }
        break;
    case 2:
        for (i = 0; i < n; i++) {
            out[i] = (unsigned char)(st->cur[i] - st->prev[i]);
        }
        break;
    default:
        /* already holds the Paeth result */
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

static const unsigned char png_sig[8] = {
    0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A
};

int micropng_begin(micropng_t *st, int width, int height,
                   micropng_write_fn write, void *user)
{
    unsigned char ihdr[13];

    if (!st || !write || width < 1 || width > MICROPNG_MAX_WIDTH ||
        height < 1) {
        return -1;
    }
    memset(st, 0, sizeof(*st));
    st->write   = write;
    st->user    = user;
    st->width   = width;
    st->height  = height;
    st->adler_a = 1;
    crc_init(st);

    emit(st, png_sig, sizeof(png_sig));

    be32(ihdr, (unsigned int)width);
    be32(ihdr + 4, (unsigned int)height);
    ihdr[8]  = 8;    /* bit depth            */
    ihdr[9]  = 2;    /* colour type: truecolour */
    ihdr[10] = 0;    /* deflate              */
    ihdr[11] = 0;    /* adaptive filtering   */
    ihdr[12] = 0;    /* no interlace         */
    chunk(st, "IHDR", ihdr, sizeof(ihdr));

    /* zlib header, then one fixed-Huffman block for the whole image. */
    idat_byte(st, 0x78);
    idat_byte(st, 0x01);
    put_bits(st, 1, 1);   /* BFINAL */
    put_bits(st, 1, 2);   /* BTYPE = fixed Huffman */

    return st->failed ? -1 : 0;
}

int micropng_row(micropng_t *st, const unsigned char *rgb)
{
    if (!st || !rgb || st->failed || st->rows_done >= st->height) {
        return -1;
    }
    memcpy(st->cur, rgb, (unsigned int)(st->width * BPP));
    filter_row(st);
    feed(st, st->filt, (unsigned int)(st->width * BPP + 1));
    memcpy(st->prev, st->cur, (unsigned int)(st->width * BPP));
    st->rows_done++;
    return st->failed ? -1 : 0;
}

int micropng_end(micropng_t *st)
{
    unsigned char adler[4];

    if (!st || st->failed) {
        return -1;
    }
    compress_window(st, 1);
    put_symbol(st, 256);              /* end of block */
    if (st->bitcnt > 0) {
        put_bits(st, 0, 8 - st->bitcnt);
    }
    be32(adler, (st->adler_b << 16) | st->adler_a);
    idat_byte(st, adler[0]);
    idat_byte(st, adler[1]);
    idat_byte(st, adler[2]);
    idat_byte(st, adler[3]);
    idat_flush(st);

    chunk(st, "IEND", 0, 0);
    return st->failed ? -1 : 0;
}
