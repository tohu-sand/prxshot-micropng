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

#ifndef PNG_WRITE_H
#define PNG_WRITE_H

#include "micropng.h"

/* Working set handed to pngWrite: encoder state plus one RGB row. */
/* Encoder state plus a full RGB snapshot of the frame. */
#define PNG_SCRATCH_SIZE (sizeof(micropng_t) + 480 * 272 * 3)

/* stride is the framebuffer width in pixels (bufferwidth), normally 512. */
int pngWrite(void *frame_addr, void *scratch, int stride, int format,
             const char *file);

#endif /* PNG_WRITE_H */
