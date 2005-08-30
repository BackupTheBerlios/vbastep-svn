// -*- C++ -*-
// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2005 Forgotten and the VBA development team

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifndef VBA_GBA_H
#define VBA_GBA_H

#include "System.h"

#define SAVE_GAME_VERSION_1 1
#define SAVE_GAME_VERSION_2 2
#define SAVE_GAME_VERSION_3 3
#define SAVE_GAME_VERSION_4 4
#define SAVE_GAME_VERSION_5 5
#define SAVE_GAME_VERSION_6 6
#define SAVE_GAME_VERSION_7 7
#define SAVE_GAME_VERSION_8 8
#define SAVE_GAME_VERSION  SAVE_GAME_VERSION_8

typedef struct {
  u8 *address;
  u32 mask;
} memoryMap;

typedef union {
  struct {
#ifdef WORDS_BIGENDIAN
    u8 B3;
    u8 B2;
    u8 B1;
    u8 B0;
#else
    u8 B0;
    u8 B1;
    u8 B2;
    u8 B3;
#endif
  } B;
  struct {
#ifdef WORDS_BIGENDIAN
    u16 W1;
    u16 W0;
#else
    u16 W0;
    u16 W1;
#endif
  } W;
#ifdef WORDS_BIGENDIAN
  volatile u32 I;
#else
	u32 I;
#endif
} reg_pair;

#ifndef NO_GBA_MAP
extern memoryMap map[256];
#endif

extern reg_pair reg[45];
extern u8 biosProtected[4];

extern char N_FLAG;
extern char Z_FLAG;
extern char C_FLAG;
extern char V_FLAG;
extern char armIrqEnable;
extern char armState;
extern int armMode;
extern void (*cpuSaveGameFunc)(u32,u8);

extern u8 freezeWorkRAM[0x40000];
extern u8 freezeInternalRAM[0x8000];
#ifdef __cplusplus
extern "C" {
#endif
extern char CPUReadGSASnapshot(const char *);
extern char CPUWriteGSASnapshot(const char *, const char *, const char *, const char *);
extern char CPUWriteBatteryFile(const char *);
extern char CPUReadBatteryFile(const char *);
extern char CPUExportEepromFile(const char *);
extern char CPUImportEepromFile(const char *);
extern char CPUWritePNGFile(const char *);
extern char CPUWriteBMPFile(const char *);
extern void CPUCleanUp();
extern void CPUUpdateRender();
extern char CPUReadMemState(char *, int);
extern char CPUReadState(const char *);
extern char CPUWriteMemState(char *, int);
extern char CPUWriteState(const char *);
extern int CPULoadRom(const char *);
extern void doMirroring(char);
extern void CPUUpdateRegister(u32, u16);
extern void CPUWriteHalfWord(u32, u16);
extern void CPUWriteByte(u32, u8);
extern void CPUInit(const char *,char);
extern void CPUReset();
extern void CPULoop(int);
extern void CPUCheckDMA(int,int);
extern char CPUIsGBAImage(const char *);
extern char CPUIsZipFile(const char *);
#ifdef PROFILING
extern void cpuProfil(char *buffer, int, u32, int);
extern void cpuEnableProfiling(int hz);
#endif

#ifdef __cplusplus
}
#endif

extern struct EmulatedSystem GBASystem;

#define R13_IRQ  18
#define R14_IRQ  19
#define SPSR_IRQ 20
#define R13_USR  26
#define R14_USR  27
#define R13_SVC  28
#define R14_SVC  29
#define SPSR_SVC 30
#define R13_ABT  31
#define R14_ABT  32
#define SPSR_ABT 33
#define R13_UND  34
#define R14_UND  35
#define SPSR_UND 36
#define R8_FIQ   37
#define R9_FIQ   38
#define R10_FIQ  39
#define R11_FIQ  40
#define R12_FIQ  41
#define R13_FIQ  42
#define R14_FIQ  43
#define SPSR_FIQ 44

#include "Cheats.h"
#include "Globals.h"
#include "EEprom.h"
#include "Flash.h"

#endif //VBA_GBA_H
