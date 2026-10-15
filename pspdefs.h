/*
 *  prxshot module
 *
 *  Copyright (C) 2011  Codestation
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

#ifndef PSPDEFS_H_
#define PSPDEFS_H_

#include <psptypes.h>

#define PSP_MEMORY_PARTITION_UMDCACHE 8

#define PSP_MODEL_PHAT 0
#define PSP_MODEL_SLIM 1
#define PSP_MODEL_GO 4

typedef struct SceModule2 {
    struct SceModule2 *next; /* 0x00 */
    u16 attribute; /* 0x04 */
    u8 version[2]; /* 0x06 */
    char modname[27]; /* 0x08 */
    char terminal; /* 0x23 */
    u16 status; /* 0x24: Module manager stage. */
    u16 padding; /* 0x26 */
    u32 unk_28; /* 0x28 */
    SceUID modid; /* 0x2C */
    SceUID usermod_thid; /* 0x30 */
    SceUID memid; /* 0x34 */
    SceUID mpidtext; /* 0x38 */
    SceUID mpiddata; /* 0x3C */
    void *ent_top; /* 0x40 */
    u32 ent_size; /* 0x44 */
    void *stub_top; /* 0x48 */
    u32 stub_size; /* 0x4C */
    int (*module_start)(SceSize, void *); /* 0x50 */
    int (*module_stop)(SceSize, void *); /* 0x54 */
    int (*module_bootstart)(SceSize, void *); /* 0x58 */
    int (*module_reboot_before)(void *); /* 0x5C */
    int (*module_reboot_phase)(SceSize, void *); /* 0x60 */
    u32 entry_addr; /* 0x64 */
    u32 gp_value; /* 0x68 */
    u32 text_addr; /* 0x6C */
    u32 text_size; /* 0x70 */
    u32 data_size; /* 0x74 */
    u32 bss_size; /* 0x78 */
    u8 nsegment; /* 0x7C */
    u8 padding2[3]; /* 0x7D */
    u32 segmentaddr[4]; /* 0x80 */
    u32 segmentsize[4]; /* 0x90 */
    int module_start_thread_priority; /* 0xA0 */
    SceSize module_start_thread_stacksize; /* 0xA4 */
    SceUInt module_start_thread_attr; /* 0xA8 */
    int module_stop_thread_priority; /* 0xAC */
    SceSize module_stop_thread_stacksize; /* 0xB0 */
    SceUInt module_stop_thread_attr; /* 0xB4 */
    int module_reboot_before_thread_priority; /* 0xB8 */
    SceSize module_reboot_before_thread_stacksize; /* 0xBC */
    SceUInt module_reboot_before_thread_attr; /* 0xC0 */
} SceModule2;

typedef int (*STMOD_HANDLER)(SceModule2 *);

int sceKernelGetModel();
char *sceKernelGetUMDData(void);
STMOD_HANDLER sctrlHENSetStartModuleHandler(STMOD_HANDLER handler);

#endif /* PSPDEFS_H_ */
