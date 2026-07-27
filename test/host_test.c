/* Host verification for micropng: encode, then compare against a reference
   decoder (see verify.py). Not built for the PSP. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "micropng.h"

static int wr(void *user, const void *data, unsigned int len)
{
    return (int)fwrite(data, 1, len, (FILE *)user);
}

int main(int argc, char **argv)
{
    /* argv: <raw rgb in> <w> <h> <png out> */
    int w = atoi(argv[2]), h = atoi(argv[3]);
    FILE *fi = fopen(argv[1], "rb");
    FILE *fo = fopen(argv[4], "wb");
    static micropng_t st;          /* ~100KB: far too big for a stack */
    unsigned char *row = malloc((size_t)w * 3);
    int y;

    if (!fi || !fo || !row) { fprintf(stderr, "open failed\n"); return 1; }
    if (micropng_begin(&st, w, h, wr, fo) != 0) { fprintf(stderr, "begin\n"); return 1; }
    for (y = 0; y < h; y++) {
        if (fread(row, 1, (size_t)w * 3, fi) != (size_t)w * 3) { fprintf(stderr, "short read\n"); return 1; }
        if (micropng_row(&st, row) != 0) { fprintf(stderr, "row %d\n", y); return 1; }
    }
    if (micropng_end(&st) != 0) { fprintf(stderr, "end\n"); return 1; }
    fprintf(stderr, "state size: %u bytes\n", (unsigned)sizeof(st));
    fclose(fi); fclose(fo); free(row);
    return 0;
}
