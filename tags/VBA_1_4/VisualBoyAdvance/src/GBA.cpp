/*
 * VisualBoyAdvanced - Nintendo Gameboy/GameboyAdvance (TM) emulator
 * Copyrigh(c) 1999-2002 Forgotten (vb@emuhq.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>
#include <string.h>

#include "GBA.h"
#include "GBAinline.h"
#include "Globals.h"
#include "Gfx.h"
#include "EEprom.h"
#include "Flash.h"
#include "Sound.h"
#include "Sram.h"
#include "bios.h"
#include "unzip.h"
#include "Cheats.h"
#include "NLS.h"
#include "elf.h"
#include "Util.h"
#include "Port.h"
#ifdef PROFILING
#include "prof/prof.h"
#endif

#define UPDATE_REG(address, value) *((u16 *)&ioMem[address]) = TO16LE(value)

#ifdef __GNUC__
#define _stricmp strcasecmp
#endif

#define CPU_BREAK_LOOP \
  cpuSavedTicks = cpuSavedTicks - *extCpuLoopTicks;\
  *extCpuLoopTicks = *extClockTicks;

#define CPU_BREAK_LOOP_2 \
  cpuSavedTicks = cpuSavedTicks - *extCpuLoopTicks;\
  *extCpuLoopTicks = *extClockTicks;\
  *extTicks = *extClockTicks;

extern int emulating;

int cpuDmaTicksToUpdate = 0;
int cpuDmaCount = 0;
int dummyAddress = 0;

int *extCpuLoopTicks = NULL;
int *extClockTicks = NULL;
int *extTicks = NULL;

int gbaSaveType = 0; // used to remember the save type on reset
bool intState = false;
bool stopState = false;
bool holdState = false;
int holdType = 0;
bool cpuSramEnabled = true;
bool cpuFlashEnabled = true;
bool cpuEEPROMEnabled = true;
bool cpuEEPROMSensorEnabled = false;

#ifdef PROFILING
int profilingTicks = 0;
int profilingTicksReload = 0;
static char *profilBuffer = NULL;
static int profilSize = 0;
static u32 profilLowPC = 0;
static int profilScale = 0;
#endif
bool freezeWorkRAM[0x40000];
bool freezeInternalRAM[0x8000];
int lcdTicks = 960;
bool timer0On = false;
int timer0Ticks = 0;
int timer0Reload = 0;
int timer0ClockReload  = 0;
bool timer1On = false;
int timer1Ticks = 0;
int timer1Reload = 0;
int timer1ClockReload  = 0;
bool timer2On = false;
int timer2Ticks = 0;
int timer2Reload = 0;
int timer2ClockReload  = 0;
bool timer3On = false;
int timer3Ticks = 0;
int timer3Reload = 0;
int timer3ClockReload  = 0;
u32 dma0SourceReload = 0;
u32 dma0DestReload = 0;
u32 dma1SourceReload = 0;
u32 dma1DestReload = 0;
u32 dma2SourceReload = 0;
u32 dma2DestReload = 0;
u32 dma3SourceReload = 0;
u32 dma3DestReload = 0;
void (*cpuSaveGameFunc)(u32,u8) = flashSaveDecide;
void (*renderLine)() = mode0RenderLine;
bool fxOn = false;
bool windowOn = false;
int frameCount = 0;
char buffer[1024];
FILE *out = NULL;
u32 lastTime = 0;
int count = 0;

int capture = 0;
int capturePrevious = 0;
int captureNumber = 0;

const int TIMER_TICKS[4] = {
  1,
  64,
  256,
  1024
};

const int thumbCycles[] = {
//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 1
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 2
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 3
    1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // 4
    2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // 5
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,  // 6
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,  // 7
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,  // 8
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,  // 9
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // a
    1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 2, 4, 1, 1,  // b
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // c
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3,  // d
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // e
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2   // f
};

const int gamepakRamWaitState[4] = { 4, 3, 2, 8 };
const int gamepakWaitState[8] =  { 4, 3, 2, 8, 4, 3, 2, 8 };
const int gamepakWaitState0[8] = { 2, 2, 2, 2, 1, 1, 1, 1 };
const int gamepakWaitState1[8] = { 4, 4, 4, 4, 1, 1, 1, 1 };
const int gamepakWaitState2[8] = { 8, 8, 8, 8, 1, 1, 1, 1 };

int memoryWait[16] =
  { 3, 0, 3, 0, 0, 1, 1, 0, 4, 4, 4, 4, 4, 4, 4, 0 };
int memoryWait32[16] =
  { 6, 0, 6, 0, 0, 2, 2, 0, 8, 8, 8, 8, 8, 8, 8, 0 };
int memoryWaitSeq[16] =
  { 1, 0, 3, 0, 0, 1, 1, 0, 2, 2, 4, 4, 8, 8, 4, 0 };
int memoryWaitSeq32[16] =
  { 2, 0, 3, 0, 0, 2, 2, 0, 4, 4, 8, 8, 16, 16, 8, 0 };
int memoryWaitFetch[16] =
  { 3, 0, 3, 0, 0, 1, 1, 0, 4, 4, 4, 4, 4, 4, 4, 0 };
int memoryWaitFetch32[16] =
  { 6, 0, 6, 0, 0, 2, 2, 0, 8, 8, 8, 8, 8, 8, 8, 0 };
const int cpuMemoryWait[16] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  2, 2, 2, 2, 2, 2, 0, 0
};
  
const bool memory32[16] =
  { false, false, false, true, true, false, false, true, false, false, false, false, false, false, false, false};

u8 biosProtected[4];

#ifdef WORDS_BIGENDIAN
bool cpuBiosSwapped = false;
#endif

u32 myROM[] = {
0xEA000006,
0x00000000,
0xEA000006,
0x00000000,
0x00000000,
0x00000000,
0xEA000088,
0x00000000,
0xE3A00302,
0xE1A0F000,
0xE92D5800,
0xE55EC002,
0xE28FB03C,
0xE79BC10C,
0xE14FB000,
0xE92D0800,
0xE20BB080,
0xE38BB01F,
0xE129F00B,
0xE92D4004,
0xE1A0E00F,
0xE12FFF1C,
0xE8BD4004,
0xE3A0C0D3,
0xE129F00C,
0xE8BD0800,
0xE169F00B,
0xE8BD5800,
0xE1B0F00E,
0x0000009C,
0x0000009C,
0x0000009C,
0x0000009C,
0x000001F8,
0x000001F0,
0x000000AC,
0x000000A0,
0x000000FC,
0x00000168,
0xE12FFF1E,
0xE1A03000,
0xE1A00001,
0xE1A01003,
0xE2113102,
0x42611000,
0xE033C040,
0x22600000,
0xE1B02001,
0xE15200A0,
0x91A02082,
0x3AFFFFFC,
0xE1500002,
0xE0A33003,
0x20400002,
0xE1320001,
0x11A020A2,
0x1AFFFFF9,
0xE1A01000,
0xE1A00003,
0xE1B0C08C,
0x22600000,
0x42611000,
0xE12FFF1E,
0xE92D0010,
0xE1A0C000,
0xE3A01001,
0xE1500001,
0x81A000A0,
0x81A01081,
0x8AFFFFFB,
0xE1A0000C,
0xE1A04001,
0xE3A03000,
0xE1A02001,
0xE15200A0,
0x91A02082,
0x3AFFFFFC,
0xE1500002,
0xE0A33003,
0x20400002,
0xE1320001,
0x11A020A2,
0x1AFFFFF9,
0xE0811003,
0xE1B010A1,
0xE1510004,
0x3AFFFFEE,
0xE1A00004,
0xE8BD0010,
0xE12FFF1E,
0xE0010090,
0xE1A01741,
0xE2611000,
0xE3A030A9,
0xE0030391,
0xE1A03743,
0xE2833E39,
0xE0030391,
0xE1A03743,
0xE2833C09,
0xE283301C,
0xE0030391,
0xE1A03743,
0xE2833C0F,
0xE28330B6,
0xE0030391,
0xE1A03743,
0xE2833C16,
0xE28330AA,
0xE0030391,
0xE1A03743,
0xE2833A02,
0xE2833081,
0xE0030391,
0xE1A03743,
0xE2833C36,
0xE2833051,
0xE0030391,
0xE1A03743,
0xE2833CA2,
0xE28330F9,
0xE0000093,
0xE1A00840,
0xE12FFF1E,
0xE3A00001,
0xE3A01001,
0xE92D4010,
0xE3A0C301,
0xE3A03000,
0xE3A04001,
0xE3500000,
0x1B000004,
0xE5CC3301,
0xEB000002,
0x0AFFFFFC,
0xE8BD4010,
0xE12FFF1E,
0xE5CC3208,
0xE15C20B8,
0xE0110002,
0x10200002,
0x114C00B8,
0xE5CC4208,
0xE12FFF1E,
0xE92D500F,
0xE3A00301,
0xE1A0E00F,
0xE510F004,
0xE8BD500F,
0xE25EF004
};

variable_desc saveGameStruct[] = {
  { &DISPCNT  , sizeof(u16) },
  { &DISPSTAT , sizeof(u16) },
  { &VCOUNT   , sizeof(u16) },
  { &BG0CNT   , sizeof(u16) },
  { &BG1CNT   , sizeof(u16) },
  { &BG2CNT   , sizeof(u16) },
  { &BG3CNT   , sizeof(u16) },
  { &BG0HOFS  , sizeof(u16) },
  { &BG0VOFS  , sizeof(u16) },
  { &BG1HOFS  , sizeof(u16) },
  { &BG1VOFS  , sizeof(u16) },
  { &BG2HOFS  , sizeof(u16) },
  { &BG2VOFS  , sizeof(u16) },
  { &BG3HOFS  , sizeof(u16) },
  { &BG3VOFS  , sizeof(u16) },
  { &BG2PA    , sizeof(u16) },
  { &BG2PB    , sizeof(u16) },
  { &BG2PC    , sizeof(u16) },
  { &BG2PD    , sizeof(u16) },
  { &BG2X_L   , sizeof(u16) },
  { &BG2X_H   , sizeof(u16) },
  { &BG2Y_L   , sizeof(u16) },
  { &BG2Y_H   , sizeof(u16) },
  { &BG3PA    , sizeof(u16) },
  { &BG3PB    , sizeof(u16) },
  { &BG3PC    , sizeof(u16) },
  { &BG3PD    , sizeof(u16) },
  { &BG3X_L   , sizeof(u16) },
  { &BG3X_H   , sizeof(u16) },
  { &BG3Y_L   , sizeof(u16) },
  { &BG3Y_H   , sizeof(u16) },
  { &WIN0H    , sizeof(u16) },
  { &WIN1H    , sizeof(u16) },
  { &WIN0V    , sizeof(u16) },
  { &WIN1V    , sizeof(u16) },
  { &WININ    , sizeof(u16) },
  { &WINOUT   , sizeof(u16) },
  { &MOSAIC   , sizeof(u16) },
  { &BLDMOD   , sizeof(u16) },
  { &COLEV    , sizeof(u16) },
  { &COLY     , sizeof(u16) },
  { &DM0SAD_L , sizeof(u16) },
  { &DM0SAD_H , sizeof(u16) },
  { &DM0DAD_L , sizeof(u16) },
  { &DM0DAD_H , sizeof(u16) },
  { &DM0CNT_L , sizeof(u16) },
  { &DM0CNT_H , sizeof(u16) },
  { &DM1SAD_L , sizeof(u16) },
  { &DM1SAD_H , sizeof(u16) },
  { &DM1DAD_L , sizeof(u16) },
  { &DM1DAD_H , sizeof(u16) },
  { &DM1CNT_L , sizeof(u16) },
  { &DM1CNT_H , sizeof(u16) },
  { &DM2SAD_L , sizeof(u16) },
  { &DM2SAD_H , sizeof(u16) },
  { &DM2DAD_L , sizeof(u16) },
  { &DM2DAD_H , sizeof(u16) },
  { &DM2CNT_L , sizeof(u16) },
  { &DM2CNT_H , sizeof(u16) },
  { &DM3SAD_L , sizeof(u16) },
  { &DM3SAD_H , sizeof(u16) },
  { &DM3DAD_L , sizeof(u16) },
  { &DM3DAD_H , sizeof(u16) },
  { &DM3CNT_L , sizeof(u16) },
  { &DM3CNT_H , sizeof(u16) },
  { &TM0D     , sizeof(u16) },
  { &TM0CNT   , sizeof(u16) },
  { &TM1D     , sizeof(u16) },
  { &TM1CNT   , sizeof(u16) },
  { &TM2D     , sizeof(u16) },
  { &TM2CNT   , sizeof(u16) },
  { &TM3D     , sizeof(u16) },
  { &TM3CNT   , sizeof(u16) },
  { &P1       , sizeof(u16) },
  { &IE       , sizeof(u16) },
  { &IF       , sizeof(u16) },
  { &IME      , sizeof(u16) },
  { &holdState, sizeof(bool) },
  { &holdType, sizeof(int) },
  { &lcdTicks, sizeof(int) },
  { &timer0On , sizeof(bool) },
  { &timer0Ticks , sizeof(int) },
  { &timer0Reload , sizeof(int) },
  { &timer0ClockReload  , sizeof(int) },
  { &timer1On , sizeof(bool) },
  { &timer1Ticks , sizeof(int) },
  { &timer1Reload , sizeof(int) },
  { &timer1ClockReload  , sizeof(int) },
  { &timer2On , sizeof(bool) },
  { &timer2Ticks , sizeof(int) },
  { &timer2Reload , sizeof(int) },
  { &timer2ClockReload  , sizeof(int) },
  { &timer3On , sizeof(bool) },
  { &timer3Ticks , sizeof(int) },
  { &timer3Reload , sizeof(int) },
  { &timer3ClockReload  , sizeof(int) },
  { &dma0SourceReload , sizeof(u32) },
  { &dma0DestReload , sizeof(u32) },
  { &dma1SourceReload , sizeof(u32) },
  { &dma1DestReload , sizeof(u32) },
  { &dma2SourceReload , sizeof(u32) },
  { &dma2DestReload , sizeof(u32) },
  { &dma3SourceReload , sizeof(u32) },
  { &dma3DestReload , sizeof(u32) },
  { &fxOn, sizeof(bool) },
  { &windowOn, sizeof(bool) },
  { &N_FLAG , sizeof(bool) },
  { &C_FLAG , sizeof(bool) },
  { &Z_FLAG , sizeof(bool) },
  { &V_FLAG , sizeof(bool) },
  { &armState , sizeof(bool) },
  { &armIrqEnable , sizeof(bool) },
  { &armNextPC , sizeof(u32) },
  { &armMode , sizeof(int) },
  { &saveType , sizeof(int) },
  { NULL, 0 } 
};

//int cpuLoopTicks = 0;
int cpuSavedTicks = 0;

#ifdef PROFILING
void cpuProfil(char *buf, int size, u32 lowPC, int scale)
{
  profilBuffer = buf;
  profilSize = size;
  profilLowPC = lowPC;
  profilScale = scale;
}

void cpuEnableProfiling(int hz)
{
  if(hz == 0)
    hz = 100;
  profilingTicks = profilingTicksReload = 16777216 / hz;
  profSetHertz(hz);
}
#endif

inline int CPUUpdateTicksAccess32(u32 address)
{
  return memoryWait32[(address>>24)&15];
}

inline int CPUUpdateTicksAccess16(u32 address)
{
  return memoryWait[(address>>24)&15];
}

inline int CPUUpdateTicksAccessSeq32(u32 address)
{
  return memoryWaitSeq32[(address>>24)&15];
}

inline int CPUUpdateTicksAccessSeq16(u32 address)
{
  return memoryWaitSeq[(address>>24)&15];
}

inline void CPUUpdateTicks(int &cpuLoopTicks)
{
  cpuLoopTicks = lcdTicks;
  
  if(soundTicks < cpuLoopTicks)
    cpuLoopTicks = soundTicks;
  
  if(timer0On && (timer0Ticks < cpuLoopTicks)) {
    cpuLoopTicks = timer0Ticks;
  }
  if(timer1On && !(TM1CNT & 4) && (timer1Ticks < cpuLoopTicks)) {
    cpuLoopTicks = timer1Ticks;
  }
  if(timer2On && !(TM2CNT & 4) && (timer2Ticks < cpuLoopTicks)) {
    cpuLoopTicks = timer2Ticks;
  }
  if(timer3On && !(TM3CNT & 4) && (timer3Ticks < cpuLoopTicks)) {
    cpuLoopTicks = timer3Ticks;
  }
#ifdef PROFILING
  if(profilingTicksReload != 0) {
    if(profilingTicks < cpuLoopTicks) {
      cpuLoopTicks = profilingTicks;
    }
  }
#endif
  cpuSavedTicks = cpuLoopTicks;
}

void CPUUpdateWindow0()
{
  int x00 = WIN0H>>8;
  int x01 = WIN0H & 255;

  if(x00 <= x01) {
    for(int i = 0; i < 240; i++) {
      gfxInWin0[i] = (i >= x00 && i < x01);
    }
  } else {
    for(int i = 0; i < 240; i++) {
      gfxInWin0[i] = (i >= x00 || i < x01);
    }
  }
}

void CPUUpdateWindow1()
{
  int x00 = WIN1H>>8;
  int x01 = WIN1H & 255;

  if(x00 <= x01) {
    for(int i = 0; i < 240; i++) {
      gfxInWin1[i] = (i >= x00 && i < x01);
    }
  } else {
    for(int i = 0; i < 240; i++) {
      gfxInWin1[i] = (i >= x00 || i < x01);
    }
  }
}

void CPUWriteInt(gzFile gzFile, int i)
{
  gzwrite(gzFile, &i, sizeof(int));
}

int CPUReadInt(gzFile gzFile)
{
  int i = 0;
  gzread(gzFile, &i, sizeof(int));
  return i;
}

void CPUReadData(gzFile gzFile, variable_desc* data)
{
  while(data->address) {
    gzread(gzFile, data->address, data->size);
    data++;
  }
}

void CPUWriteData(gzFile gzFile, variable_desc *data)
{
  while(data->address) {
    gzwrite(gzFile, data->address, data->size);
    data++;
  }
}

bool CPUWriteState(char *file)
{
  gzFile gzFile = gzopen(file, "wb");

  if(gzFile == NULL) {
    systemMessage(MSG_ERROR_CREATING_FILE, "Error creating file %s", file);
    return false;
  }

  CPUWriteInt(gzFile, SAVE_GAME_VERSION);

  gzwrite(gzFile, &rom[0xa0], 16);

  CPUWriteInt(gzFile, useBios);
  
  gzwrite(gzFile, &reg[0], sizeof(reg));

  CPUWriteData(gzFile, saveGameStruct);

  // new to version 0.7.1
  CPUWriteInt(gzFile, stopState);
  // new to version 0.8
  CPUWriteInt(gzFile, intState);

  gzwrite(gzFile, internalRAM, 0x8000);
  gzwrite(gzFile, paletteRAM, 0x400);
  gzwrite(gzFile, workRAM, 0x40000);
  gzwrite(gzFile, vram, 0x20000);
  gzwrite(gzFile, oam, 0x400);
  gzwrite(gzFile, pix, 4*241*162);
  gzwrite(gzFile, ioMem, 0x400);

  eepromSaveGame(gzFile);
  flashSaveGame(gzFile);
  soundSaveGame(gzFile);

  cheatsSaveGame(gzFile);
  
  gzclose(gzFile);
  
  return true;
}

bool CPUReadState(char * file)
{
  gzFile gzFile = gzopen(file, "rb");

  if(gzFile == NULL)
    return false;

  int version = CPUReadInt(gzFile);

  if(version > SAVE_GAME_VERSION || version < SAVE_GAME_VERSION_1) {
    systemMessage(MSG_UNSUPPORTED_VBA_SGM,
                  "Unsupported VisualBoyAdvance save game version %d",
                  version);
    gzclose(gzFile);
    return false;
  }
  
  u8 romname[17];

  gzread(gzFile, romname, 16);

  if(memcmp(&rom[0xa0], romname, 16) != 0) {
    romname[16]=0;
    for(int i = 0; i < 16; i++)
      if(romname[i] < 32)
        romname[i] = 32;
    systemMessage(MSG_CANNOT_LOAD_SGM,"Cannot load save game for %s", romname);
    gzclose(gzFile);
    return false;
  }

  bool ub = CPUReadInt(gzFile) ? true : false;

  if(ub != useBios) {
    if(useBios)
      systemMessage(MSG_SAVE_GAME_NOT_USING_BIOS,
                    "Save game is not using the BIOS files");
    else
      systemMessage(MSG_SAVE_GAME_USING_BIOS,
                    "Save game is using the BIOS file");
    gzclose(gzFile);
    return false;
  }

  gzread(gzFile, &reg[0], sizeof(reg));

  CPUReadData(gzFile, saveGameStruct);

  if(version < SAVE_GAME_VERSION_3)
    stopState = false;
  else
    stopState = CPUReadInt(gzFile) ? true : false;

  if(version < SAVE_GAME_VERSION_4)
    intState = false;
  else
    intState = CPUReadInt(gzFile) ? true : false;
  
  gzread(gzFile, internalRAM, 0x8000);
  gzread(gzFile, paletteRAM, 0x400);
  gzread(gzFile, workRAM, 0x40000);
  gzread(gzFile, vram, 0x20000);
  gzread(gzFile, oam, 0x400);
  if(version < SAVE_GAME_VERSION_6)
    gzread(gzFile, pix, 4*240*160);
  else
    gzread(gzFile, pix, 4*241*162);
  gzread(gzFile, ioMem, 0x400);

  eepromReadGame(gzFile, version);
  flashReadGame(gzFile, version);
  soundReadGame(gzFile, version);

  // set pointers!
  CPUUpdateRender();
  CPUUpdateWindow0();
  CPUUpdateWindow1();
  gbaSaveType = 0;
  switch(saveType) {
  case 0:
    cpuSaveGameFunc = flashSaveDecide;
    break;
  case 1:
    cpuSaveGameFunc = sramWrite;
    gbaSaveType = 1;
    break;
  case 2:
    cpuSaveGameFunc = flashWrite;
    gbaSaveType = 2;
    break;
  default:
    systemMessage(MSG_UNSUPPORTED_SAVE_TYPE,
                  "Unsupported save type %d", saveType);
    break;
  }
  if(eepromInUse)
    gbaSaveType = 3;
  
  if(version > SAVE_GAME_VERSION_1) {
    cheatsReadGame(gzFile);
  }
  gzclose(gzFile);

  layerEnable = layerSettings & DISPCNT;
  
  return true;  
}

bool CPUExportEepromFile(char *fileName)
{
  if(eepromInUse) {
    FILE *file = fopen(fileName, "wb");
    
    if(!file) {
      systemMessage(MSG_ERROR_CREATING_FILE, "Error creating file %s",
                    fileName);
      return false;
    }

    for(int i = 0; i < eepromSize;) {
      for(int j = 0; j < 8; j++) {
        if(fwrite(&eepromData[i+7-j], 1, 1, file) != 1) {
          fclose(file);
          return false;
        }
      }
      i += 8;
    }
    fclose(file);
  }
  return true;
}

bool CPUWriteBatteryFile(char *fileName)
{
  if(gbaSaveType == 0) {
    if(eepromInUse)
      gbaSaveType = 3;
    else switch(saveType) {
    case 1:
      gbaSaveType = 1;
      break;
    case 2:
      gbaSaveType = 2;
      break;
    }
  }
  
  if(gbaSaveType) {
    FILE *file = fopen(fileName, "wb");
    
    if(!file) {
      systemMessage(MSG_ERROR_CREATING_FILE, "Error creating file %s",
                    fileName);
      return false;
    }
    
    // only save if Flash/Sram in use or EEprom in use
    if(gbaSaveType != 3) {
      if(gbaSaveType == 2) {
        if(fwrite(flashSaveMemory, 1, flashSize, file) != (size_t)flashSize) {
          fclose(file);
          return false;
        }
      } else {
        if(fwrite(flashSaveMemory, 1, 0x10000, file) != 0x10000) {
          fclose(file);
          return false;
        }
      }
    } else {
      if(fwrite(eepromData, 1, eepromSize, file) != (size_t)eepromSize) {
        fclose(file);
        return false;
      }
    }
    fclose(file);
  }
  return true;
}

bool CPUReadGSASnapshot(char *fileName)
{
  int i;
  FILE *file = fopen(fileName, "rb");
    
  if(!file) {
    systemMessage(MSG_CANNOT_OPEN_FILE, "Cannot open file %s", fileName);
    return false;
  }
  
  // check file size to know what we should read
  fseek(file, 0, SEEK_END);

  // long size = ftell(file);
  fseek(file, 0x0, SEEK_SET);
  fread(&i, 1, 4, file);
  fseek(file, i, SEEK_CUR); // Skip SharkPortSave
  fseek(file, 4, SEEK_CUR); // skip some sort of flag
  fread(&i, 1, 4, file); // name length
  fseek(file, i, SEEK_CUR); // skip name
  fread(&i, 1, 4, file); // desc length
  fseek(file, i, SEEK_CUR); // skip desc
  fread(&i, 1, 4, file); // notes length
  fseek(file, i, SEEK_CUR); // skip notes
  int saveSize;
  fread(&saveSize, 1, 4, file); // read length
  saveSize -= 0x1c; // remove header size
  char buffer[17];
  char buffer2[17];
  fread(buffer, 1, 16, file);
  buffer[16] = 0;
  for(i = 0; i < 16; i++)
    if(buffer[i] < 32)
      buffer[i] = 32;
  memcpy(buffer2, &rom[0xa0], 16);
  buffer2[16] = 0;
  for(i = 0; i < 16; i++)
    if(buffer2[i] < 32)
      buffer2[i] = 32;  
  if(memcmp(buffer, buffer2, 16)) {
    systemMessage(MSG_CANNOT_IMPORT_SNAPSHOT_FOR,
                  "Cannot import snapshot for %s. Current game is %s",
                  buffer,
                  buffer2);
    fclose(file);
    return false;
  }
  fseek(file, 12, SEEK_CUR); // skip some flags
  if(saveSize >= 65536) {
    if(fread(flashSaveMemory, 1, saveSize, file) != (size_t)saveSize) {
      fclose(file);
      return false;
    }
  } else {
    systemMessage(MSG_UNSUPPORTED_SNAPSHOT_FILE,
                  "Unsupported snapshot file %s",
                  fileName);
    fclose(file);
    return false;
  }
  fclose(file);
  CPUReset();
  return true;
}

void CPUWriteInt(char *buffer, int value)
{
  buffer[0] = value & 255;
  buffer[1] = (value >> 8) & 255;
  buffer[2] = (value >> 16) & 255;
  buffer[3] = (value >> 24) & 255;
}

bool CPUWriteGSASnapshot(char *fileName, char *title, char *desc, char *notes)
{
  FILE *file = fopen(fileName, "wb");
    
  if(!file) {
    systemMessage(MSG_CANNOT_OPEN_FILE, "Cannot open file %s", fileName);
    return false;
  }

  char buffer[17];

  CPUWriteInt(buffer, 0x0d); // SharkPortSave length
  fwrite(buffer, 1, 4, file);
  fwrite("SharkPortSave", 1, 0x0d, file);
  CPUWriteInt(buffer, 0x000f0000);
  fwrite(buffer, 1, 4, file); // save type 0x000f0000 = GBA save
  CPUWriteInt(buffer, strlen(title));
  fwrite(buffer, 1, 4, file); // title length
  fwrite(title, 1, strlen(title), file);
  CPUWriteInt(buffer, strlen(desc));
  fwrite(buffer, 1, 4, file); // desc length
  fwrite(desc, 1, strlen(desc), file);
  CPUWriteInt(buffer, strlen(notes));
  fwrite(buffer, 1, 4, file); // notes length
  fwrite(notes, 1, strlen(notes), file);
  int saveSize = 0x10000;
  if(gbaSaveType == 2)
    saveSize = flashSize;
  int totalSize = saveSize + 0x1c;

  CPUWriteInt(buffer, totalSize); // length of remainder of save - CRC
  fwrite(buffer, 1, 4, file);

  char temp[0x2001c];
  memset(temp, 0, 28);
  memcpy(temp, &rom[0xa0], 16); // copy internal name
  temp[0x10] = rom[0xbe]; // reserved area (old checksum)
  temp[0x11] = rom[0xbf]; // reserved area (old checksum)
  temp[0x12] = rom[0xbd]; // complement check
  temp[0x13] = rom[0xb0]; // maker
  temp[0x14] = 1; // 1 save ?
  memcpy(&temp[0x1c], flashSaveMemory, saveSize); // copy save
  fwrite(temp, 1, totalSize, file); // write save + header
  u32 crc = 0;
  
  for(int i = 0; i < totalSize; i++) {
    crc += ((u32)temp[i] << (crc % 0x18));
  }
  
  CPUWriteInt(buffer, crc);
  fwrite(buffer, 1, 4, file); // CRC?
  
  fclose(file);
  return true;
}

bool CPUImportEepromFile(char *fileName)
{
  FILE *file = fopen(fileName, "rb");
    
  if(!file)
    return false;
  
  // check file size to know what we should read
  fseek(file, 0, SEEK_END);

  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  if(size == 512 || size == 0x2000) {
    if(fread(eepromData, 1, size, file) != (size_t)size) {
      fclose(file);
      return false;
    }
    for(int i = 0; i < size;) {
      u8 tmp = eepromData[i];
      eepromData[i] = eepromData[7-i];
      eepromData[7-i] = tmp;
      i++;
      tmp = eepromData[i];
      eepromData[i] = eepromData[7-i];
      eepromData[7-i] = tmp;
      i++;
      tmp = eepromData[i];
      eepromData[i] = eepromData[7-i];
      eepromData[7-i] = tmp;
      i++;      
      tmp = eepromData[i];
      eepromData[i] = eepromData[7-i];
      eepromData[7-i] = tmp;
      i++;      
      i += 4;
    }
  } else
    return false;
  fclose(file);
  return true;
}

bool CPUReadBatteryFile(char *fileName)
{
  FILE *file = fopen(fileName, "rb");
    
  if(!file)
    return false;
  
  // check file size to know what we should read
  fseek(file, 0, SEEK_END);

  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  if(size == 512 || size == 0x2000) {
    if(fread(eepromData, 1, size, file) != (size_t)size) {
      fclose(file);
      return false;
    }
  } else {
    if(size == 0x20000) {
      if(fread(flashSaveMemory, 1, 0x20000, file) != 0x20000) {
        fclose(file);
        return false;
      }
      flashSetSize(0x20000);
    } else {
      if(fread(flashSaveMemory, 1, 0x10000, file) != 0x10000) {
        fclose(file);
        return false;
      }
      flashSetSize(0x10000);
    }
  }
  fclose(file);
  return true;
}

bool CPUWritePNGFile(char *fileName)
{
  return utilWritePNGFile(fileName, 240, 160, pix);
}

bool CPUWriteBMPFile(char *fileName)
{
  return utilWriteBMPFile(fileName, 240, 160, pix);
}

bool CPUIsZipFile(char * file)
{
  if(strlen(file) > 4) {
    char * p = strrchr(file,'.');

    if(p != NULL) {
      if(_stricmp(p, ".zip") == 0)
        return true;
    }
  }

  return false;
}

bool CPUIsGBAImage(char * file)
{
  cpuIsMultiBoot = false;
  if(strlen(file) > 4) {
    char * p = strrchr(file,'.');

    if(p != NULL) {
      if(_stricmp(p, ".gba") == 0)
        return true;
      if(_stricmp(p, ".agb") == 0)
        return true;
      if(_stricmp(p, ".bin") == 0)
        return true;
      if(_stricmp(p, ".elf") == 0)
        return true;
      if(_stricmp(p, ".mb") == 0) {
        cpuIsMultiBoot = true;
        return true;
      }
    }
  }

  return false;
}

bool CPUIsELF(char *file)
{
  if(strlen(file) > 4) {
    char * p = strrchr(file,'.');
    
    if(p != NULL) {
      if(_stricmp(p, ".elf") == 0)
        return true;
    }
  }
  return false;
}

void CPUCleanUp()
{
#ifdef PROFILING
  if(profilingTicksReload) {
    profCleanup();
  }
#endif
  
  if(rom != NULL) {
    free(rom);
    rom = NULL;
  }

  if(vram != NULL) {
    free(vram);
    vram = NULL;
  }

  if(paletteRAM != NULL) {
    free(paletteRAM);
    paletteRAM = NULL;
  }
  
  if(internalRAM != NULL) {
    free(internalRAM);
    internalRAM = NULL;
  }

  if(workRAM != NULL) {
    free(workRAM);
    workRAM = NULL;
  }

  if(bios != NULL) {
    free(bios);
    bios = NULL;
  }

  if(pix != NULL) {
    free(pix);
    pix = NULL;
  }

  if(oam != NULL) {
    free(oam);
    oam = NULL;
  }

  if(ioMem != NULL) {
    free(ioMem);
    ioMem = NULL;
  }
  
  elfCleanUp();

  emulating = 0;
}



bool CPULoadRom(char *szFile)
{
  FILE * f = NULL;
  char buffer[2048];
  long size = 0;  
  
  if(rom != NULL) {
    CPUCleanUp();
  }
  
  rom = (u8 *)malloc(0x2000000);
  if(rom == NULL) {
    systemMessage(MSG_OUT_OF_MEMORY, "Failed to allocate memory for %s",
                  "ROM");
    return false;
  }
  u16 *temp = (u16 *)rom;
  int i;
  for(i = 0; i < 0x2000000; i+=2) {
    *temp++ = TO16LE((i >> 1) & 0xFFFF);
  }
  workRAM = (u8 *)calloc(1, 0x40000);
  if(workRAM == NULL) {
    systemMessage(MSG_OUT_OF_MEMORY, "Failed to allocate memory for %s",
                  "WRAM");
    return false;
  }  
  if(CPUIsZipFile(szFile)) {
    unzFile unz = unzOpen(szFile);
    
    if(unz == NULL) {
      systemMessage(MSG_CANNOT_OPEN_FILE, "Cannot open file %s", szFile);
      free(rom);
      rom = NULL;
      free(workRAM);
      workRAM = NULL;
      return false;
    }
    int r = unzGoToFirstFile(unz);
    
    if(r != UNZ_OK) {
      unzClose(unz);
      systemMessage(MSG_BAD_ZIP_FILE, "Bad ZIP file %s", szFile);
      free(rom);
      rom = NULL;
      free(workRAM);
      workRAM = NULL;
      return false;
    }
    
    bool found = false;
    
    unz_file_info info;
    
    while(true) {
      r = unzGetCurrentFileInfo(unz,
                                &info,
                                buffer,
                                sizeof(buffer),
                                NULL,
                                0,
                                NULL,
                                0);
      
      if(r != UNZ_OK) {
        unzClose(unz);
        systemMessage(MSG_BAD_ZIP_FILE,"Bad ZIP file %s", szFile);
        free(rom);
        rom = NULL;
        free(workRAM);
        workRAM = NULL;
        return false;
      }
      
      if(CPUIsGBAImage(buffer)) {
        found = true;
        break;
      }
        
      r = unzGoToNextFile(unz);
      
      if(r != UNZ_OK)
        break;
    }

    if(!found) {
      unzClose(unz);
      systemMessage(MSG_NO_IMAGE_ON_ZIP,
                    "No image found on ZIP file %s", szFile);
      free(rom);
      rom = NULL;
      free(workRAM);
      workRAM = NULL;
      return false;
    }
    
    size = info.uncompressed_size;
    
    r = unzOpenCurrentFile(unz);

    if(r != UNZ_OK) {
      unzClose(unz);
      systemMessage(MSG_ERROR_OPENING_IMAGE,"Error opening image %s", buffer);
      free(rom);
      rom = NULL;
      free(workRAM);
      workRAM = NULL;
      return false;
    }

    if(cpuIsMultiBoot)
      r = unzReadCurrentFile(unz,
                             workRAM,
                             size);
    else
      r = unzReadCurrentFile(unz,
                             rom,
                             size);

    if(r != (int)size) {
      unzCloseCurrentFile(unz);
      unzClose(unz);
      systemMessage(MSG_ERROR_READING_IMAGE,
                    "Error reading image %s", buffer);
      free(rom);
      rom = NULL;
      free(workRAM);
      workRAM = NULL;
      return false;
    }
    
    unzCloseCurrentFile(unz);
    unzClose(unz);
  } else {
    f = fopen(szFile,
              "rb");
    if(!f) {
      systemMessage(MSG_ERROR_OPENING_IMAGE, "Error opening image %s", szFile);
      free(rom);
      rom = NULL;
      free(workRAM);
      workRAM = NULL;
      return false;
    }
    
    if(CPUIsELF(szFile)) {
      if(!elfRead(szFile, f)) {
        free(rom);
        rom = NULL;
        free(workRAM);
        workRAM = NULL;
        return false;
      }
    } else {
      fseek(f,0,SEEK_END);
      size = ftell(f);
      fseek(f,0,SEEK_SET);

      if(cpuIsMultiBoot)
        fread(workRAM, 1, size, f);
      else
        fread(rom,1,size,f);
      fclose(f);
    }
  }

  bios = (u8 *)calloc(1,0x4000);
  if(bios == NULL) {
    systemMessage(MSG_OUT_OF_MEMORY, "Failed to allocate memory for %s",
                  "BIOS");
    CPUCleanUp();
    return false;
  }    
  internalRAM = (u8 *)calloc(1,0x8000);
  if(internalRAM == NULL) {
    systemMessage(MSG_OUT_OF_MEMORY, "Failed to allocate memory for %s",
                  "IRAM");
    CPUCleanUp();
    return false;
  }    
  paletteRAM = (u8 *)calloc(1,0x400);
  if(paletteRAM == NULL) {
    systemMessage(MSG_OUT_OF_MEMORY, "Failed to allocate memory for %s",
                  "PRAM");
    CPUCleanUp();
    return false;
  }      
  vram = (u8 *)calloc(1, 0x20000);
  if(vram == NULL) {
    systemMessage(MSG_OUT_OF_MEMORY, "Failed to allocate memory for %s",
                  "VRAM");
    CPUCleanUp();
    return false;
  }      
  oam = (u8 *)calloc(1, 0x400);
  if(oam == NULL) {
    systemMessage(MSG_OUT_OF_MEMORY, "Failed to allocate memory for %s",
                  "OAM");
    CPUCleanUp();
    return false;
  }      
  pix = (u8 *)calloc(1, 4 * 241 * 162);
  if(pix == NULL) {
    systemMessage(MSG_OUT_OF_MEMORY, "Failed to allocate memory for %s",
                  "PIX");
    CPUCleanUp();
    return false;
  }      
  ioMem = (u8 *)calloc(1, 0x400);
  if(ioMem == NULL) {
    systemMessage(MSG_OUT_OF_MEMORY, "Failed to allocate memory for %s",
                  "IO");
    CPUCleanUp();
    return false;
  }      

  return true;
}

void CPUUpdateRender()
{
  switch(DISPCNT & 7) {
  case 0:
    if((!fxOn && !windowOn && !(layerEnable & 0x8000)) ||
       cpuDisableSfx)
      renderLine = mode0RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode0RenderLineNoWindow;
    else 
      renderLine = mode0RenderLineAll;
    break;
  case 1:
    if((!fxOn && !windowOn && !(layerEnable & 0x8000)) ||
       cpuDisableSfx)
      renderLine = mode1RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode1RenderLineNoWindow;
    else
      renderLine = mode1RenderLineAll;
    break;
  case 2:
    if((!fxOn && !windowOn && !(layerEnable & 0x8000)) ||
       cpuDisableSfx)
      renderLine = mode2RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode2RenderLineNoWindow;
    else
      renderLine = mode2RenderLineAll;
    break;
  case 3:
    if((!fxOn && !windowOn && !(layerEnable & 0x8000)) ||
       cpuDisableSfx)
      renderLine = mode3RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode3RenderLineNoWindow;
    else
      renderLine = mode3RenderLineAll;
    break;
  case 4:
    if((!fxOn && !windowOn && !(layerEnable & 0x8000)) ||
       cpuDisableSfx)
      renderLine = mode4RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode4RenderLineNoWindow;
    else
      renderLine = mode4RenderLineAll;
    break;
  case 5:
    if((!fxOn && !windowOn && !(layerEnable & 0x8000)) ||
       cpuDisableSfx)
      renderLine = mode5RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode5RenderLineNoWindow;
    else
      renderLine = mode5RenderLineAll;
  default:
    break;
  }
}

void CPUUpdateCPSR()
{
  u32 CPSR = reg[16].I & 0x40;
  if(N_FLAG)
    CPSR |= 0x80000000;
  if(Z_FLAG)
    CPSR |= 0x40000000;
  if(C_FLAG)
    CPSR |= 0x20000000;
  if(V_FLAG)
    CPSR |= 0x10000000;
  if(!armState)
    CPSR |= 0x00000020;
  if(!armIrqEnable)
    CPSR |= 0x80;
  CPSR |= (armMode & 0x1F);
  reg[16].I = CPSR;
}

void CPUUpdateFlags(bool breakLoop)
{
  u32 CPSR = reg[16].I;
  
  N_FLAG = (CPSR & 0x80000000) ? true: false;
  Z_FLAG = (CPSR & 0x40000000) ? true: false;
  C_FLAG = (CPSR & 0x20000000) ? true: false;
  V_FLAG = (CPSR & 0x10000000) ? true: false;
  armState = (CPSR & 0x20) ? false : true;
  armIrqEnable = (CPSR & 0x80) ? false : true;
  if(breakLoop) {
    if(armIrqEnable && (IF & IE) && (IME & 1)) {
      CPU_BREAK_LOOP_2;
    }
  }
}

void CPUUpdateFlags()
{
  CPUUpdateFlags(true);
}

void CPUSwap(u32& a, u32& b)
{
  u32 c = b;
  b = a;
  a = c;
}

void CPUSwitchMode(int mode, bool saveState, bool breakLoop)
{
  //  if(armMode == mode)
  //    return;
  
  CPUUpdateCPSR();

  switch(armMode) {
  case 0x10:
  case 0x1F:
    reg[R13_USR].I = reg[13].I;
    reg[R14_USR].I = reg[14].I;
    reg[17].I = reg[16].I;
    break;
  case 0x11:
    CPUSwap(reg[R8_FIQ].I, reg[8].I);
    CPUSwap(reg[R9_FIQ].I, reg[9].I);
    CPUSwap(reg[R10_FIQ].I, reg[10].I);
    CPUSwap(reg[R11_FIQ].I, reg[11].I);
    CPUSwap(reg[R12_FIQ].I, reg[12].I);
    reg[R13_FIQ].I = reg[13].I;
    reg[R14_FIQ].I = reg[14].I;
    reg[SPSR_FIQ].I = reg[17].I;
    break;
  case 0x12:
    reg[R13_IRQ].I  = reg[13].I;
    reg[R14_IRQ].I  = reg[14].I;
    reg[SPSR_IRQ].I =  reg[17].I;
    break;
  case 0x13:
    reg[R13_SVC].I  = reg[13].I;
    reg[R14_SVC].I  = reg[14].I;
    reg[SPSR_SVC].I =  reg[17].I;
    break;
  case 0x17:
    reg[R13_ABT].I  = reg[13].I;
    reg[R14_ABT].I  = reg[14].I;
    reg[SPSR_ABT].I =  reg[17].I;
    break;
  case 0x1b:
    reg[R13_UND].I  = reg[13].I;
    reg[R14_UND].I  = reg[14].I;
    reg[SPSR_UND].I =  reg[17].I;
    break;
  }

  u32 CPSR = reg[16].I;
  u32 SPSR = reg[17].I;
  
  switch(mode) {
  case 0x10:
  case 0x1F:
    reg[13].I = reg[R13_USR].I;
    reg[14].I = reg[R14_USR].I;
    reg[16].I = SPSR;
    break;
  case 0x11:
    CPUSwap(reg[8].I, reg[R8_FIQ].I);
    CPUSwap(reg[9].I, reg[R9_FIQ].I);
    CPUSwap(reg[10].I, reg[R10_FIQ].I);
    CPUSwap(reg[11].I, reg[R11_FIQ].I);
    CPUSwap(reg[12].I, reg[R12_FIQ].I);
    reg[13].I = reg[R13_FIQ].I;
    reg[14].I = reg[R14_FIQ].I;
    if(saveState)
      reg[17].I = CPSR;
    else
      reg[17].I = reg[SPSR_FIQ].I;
    break;
  case 0x12:
    reg[13].I = reg[R13_IRQ].I;
    reg[14].I = reg[R14_IRQ].I;
    reg[16].I = SPSR;
    if(saveState)
      reg[17].I = CPSR;
    else
      reg[17].I = reg[SPSR_IRQ].I;
    break;
  case 0x13:
    reg[13].I = reg[R13_SVC].I;
    reg[14].I = reg[R14_SVC].I;
    reg[16].I = SPSR;
    if(saveState)
      reg[17].I = CPSR;
    else
      reg[17].I = reg[SPSR_SVC].I;
    break;
  case 0x17:
    reg[13].I = reg[R13_ABT].I;
    reg[14].I = reg[R14_ABT].I;
    reg[16].I = SPSR;
    if(saveState)
      reg[17].I = CPSR;
    else
      reg[17].I = reg[SPSR_ABT].I;
    break;    
  case 0x1b:
    reg[13].I = reg[R13_UND].I;
    reg[14].I = reg[R14_UND].I;
    reg[16].I = SPSR;
    if(saveState)
      reg[17].I = CPSR;
    else
      reg[17].I = reg[SPSR_UND].I;
    break;    
  default:
    systemMessage(MSG_UNSUPPORTED_ARM_MODE,"Unsupported ARM mode %02x", mode);
    break;
  }
  armMode = mode;
  CPUUpdateFlags(breakLoop);
  CPUUpdateCPSR();
}

void CPUSwitchMode(int mode, bool saveState)
{
  CPUSwitchMode(mode, saveState, true);
}

void CPUSoftwareInterrupt()
{
  u32 PC = reg[15].I;
  bool savedArmState = armState;
  CPUSwitchMode(0x13, true, false);
  reg[14].I = PC - (savedArmState ? 4 : 2);
  reg[15].I = 0x08;
  armState = true;
  armIrqEnable = false;
  armNextPC = 0x08;
  reg[15].I += 4;
}

void CPUSoftwareInterrupt(int comment)
{
  static bool disableMessage = false;
#ifdef BKPT_SUPPORT
  if(comment == 0xff || comment == 0x00ff0000) {
    extern void (*dbgOutput)(char *, u32);
    dbgOutput(NULL, reg[0].I);
    return;
  }
#endif
#ifdef PROFILING
  if(comment == 0xfe || comment == 0x00fe0000) {
    profStartup(reg[0].I, reg[1].I);
    return;
  }
  if(comment == 0xfd || comment == 0x00fd0000) {
    profControl(reg[0].I);
    return;
  }
  if(comment == 0xfc || comment == 0x00fc0000) {
    profCleanup();
    return;
  }
  if(comment == 0xfb || comment == 0x00fb0000) {
    profCount();
    return;
  }
#endif
  if(useBios) {
#ifdef DEV_VERSION
    if(systemVerbose & VERBOSE_SWI) {
      log("SWI: %08x at %08x (0x%08x,0x%08x,0x%08x,VCOUNT = %2d)\n", comment,
          armState ? armNextPC - 4: armNextPC -2,
          reg[0].I,
          reg[1].I,
          reg[2].I,
          VCOUNT);
    }
#endif
    CPUSoftwareInterrupt();
    return;
  }
  // This would be correct, but it causes problems if uncommented
  //  else {
  //    biosProtected = 0xe3a02004;
  //  }
     
  if(comment > 0xffff)
    comment >>= 16;
  switch(comment) {
  case 0x00:
    BIOS_SoftReset();
    break;
  case 0x01:
    BIOS_RegisterRamReset();
    break;
  case 0x02:
#ifdef DEV_VERSION
    if(systemVerbose & VERBOSE_SWI) {
      log("Halt: (VCOUNT = %2d)\n",
          VCOUNT);      
    }
#endif    
    holdState = true;
    holdType = -1;
    break;
  case 0x03:
#ifdef DEV_VERSION
    if(systemVerbose & VERBOSE_SWI) {
      log("Stop: (VCOUNT = %2d)\n",
          VCOUNT);      
    }
#endif    
    holdState = true;
    holdType = -1;
    stopState = true;
    break;
  case 0x04:
#ifdef DEV_VERSION
    if(systemVerbose & VERBOSE_SWI) {
      log("IntrWait: 0x%08x,0x%08x (VCOUNT = %2d)\n",
          reg[0].I,
          reg[1].I,
          VCOUNT);      
    }
#endif
    CPUSoftwareInterrupt();
    break;    
  case 0x05:
#ifdef DEV_VERSION
    if(systemVerbose & VERBOSE_SWI) {
      log("VBlankIntrWait: (VCOUNT = %2d)\n", 
          VCOUNT);      
    }
#endif
    CPUSoftwareInterrupt();
    break;
  case 0x06:
    CPUSoftwareInterrupt();
    break;
  case 0x07:
    CPUSoftwareInterrupt();
    break;
  case 0x08:
    BIOS_Sqrt();
    break;
  case 0x0A:
    BIOS_ArcTan2();
    break;
  case 0x0B:
    BIOS_CpuSet();
    break;
  case 0x0C:
    BIOS_CpuFastSet();
    break;
  case 0x0E:
    BIOS_BgAffineSet();
    break;
  case 0x0F:
    BIOS_ObjAffineSet();
    break;
  case 0x10:
    BIOS_BitUnPack();
    break;
  case 0x11:
    BIOS_LZ77UnCompWram();
    break;
  case 0x12:
    BIOS_LZ77UnCompVram();
    break;
  case 0x13:
    BIOS_HuffUnComp();
    break;
  case 0x14:
    BIOS_RLUnCompWram();
    break;
  case 0x15:
    BIOS_RLUnCompVram();
    break;
  case 0x16:
    BIOS_Diff8bitUnFilterWram();
    break;
  case 0x17:
    BIOS_Diff8bitUnFilterVram();
    break;
  case 0x18:
    BIOS_Diff16bitUnFilter();
    break;
  case 0x19:
#ifdef DEV_VERSION
    if(systemVerbose & VERBOSE_SWI) {
      log("SoundBiasSet: 0x%08x (VCOUNT = %2d)\n",
          reg[0].I,
          VCOUNT);      
    }
#endif    
    if(reg[0].I)
      systemSoundPause();
    else
      systemSoundResume();
    break;
  case 0x1F:
    BIOS_MidiKey2Freq();
    break;
  default:
#ifdef DEV_VERSION
    if(systemVerbose & VERBOSE_SWI) {
      log("SWI: %08x at %08x (0x%08x,0x%08x,0x%08x,VCOUNT = %2d)\n", comment,
          armState ? armNextPC - 4: armNextPC -2,
          reg[0].I,
          reg[1].I,
          reg[2].I,
          VCOUNT);
    }
#endif
    
    if(!disableMessage) {
      systemMessage(MSG_UNSUPPORTED_BIOS_FUNCTION,
                    "Unsupported BIOS function %02x called from %08x. A BIOS file is needed in order to get correct behaviour.",
                    comment,
                    armMode ? armNextPC - 4: armNextPC - 2);
      disableMessage = true;
    }
    break;
  }
}

void CPUCompareVCOUNT()
{
  if(VCOUNT == (DISPSTAT >> 8)) {
    DISPSTAT |= 4;
    UPDATE_REG(0x04, DISPSTAT);

    if(DISPSTAT & 0x20) {
      IF |= 4;
      UPDATE_REG(0x202, IF);
    }
  } else {
    DISPSTAT &= 0xFFFB;
    UPDATE_REG(0x4, DISPSTAT);
  }
}

void doDMA(u32 &s, u32 &d, u32 si, u32 di, u32 c, int transfer32)
{
  // int sm = s >> 24;
  // int dm = d >> 24;

  int sc = c;

  cpuDmaCount = c;
  
  if(transfer32) {
    s &= 0xFFFFFFFC;
    if(s < 0x02000000 && (reg[15].I >> 24)) {
      while(c != 0) {
        CPUWriteMemory(d, 0);
        d += di;
        c--;
      }
    } else {
      while(c != 0) {
        CPUWriteMemory(d, CPUReadMemory(s));
        d += di;
        s += si;
        c--;
      }
    }
  } else {
    s &= 0xFFFFFFFE;
    si = (int)si >> 1;
    di = (int)di >> 1;
    if(s < 0x02000000 && (reg[15].I >> 24)) {
      while(c != 0) {
        CPUWriteHalfWord(d, 0);
        d += di;
        c--;
      }
    } else {
      while(c != 0) {
        CPUWriteHalfWord(d, CPUReadHalfWord(s));
        d += di;
        s += si;
        c--;
      }
    }
  }

  cpuDmaCount = 0;
  
  //  int sw = 1+memoryWait[sm & 15];
  //  int dw = 1+memoryWait[dm & 15];

  //  int totalTicks = 0;
  
  //  if(transfer32) {
  //    if(!memory32[sm & 15])
  //      sw <<= 1;
  //    if(!memory32[dm & 15])
  //      dw <<= 1;
  //  }
  
  //  totalTicks = (sw+dw)*sc;

  int totalTicks = (transfer32 ? 3 : 2) * sc;

  cpuDmaTicksToUpdate += totalTicks;

  if(*extCpuLoopTicks >= 0) {
    CPU_BREAK_LOOP;
  }
}

void CPUCheckDMA(int reason, int dmamask)
{
  // DMA 0
  if((DM0CNT_H & 0x8000) && (dmamask & 1)) {
   if(((DM0CNT_H >> 12) & 3) == reason) {
     u32 sourceIncrement = 4;
     u32 destIncrement = 4;
     switch((DM0CNT_H >> 7) & 3) {
     case 0:
       break;
     case 1:
       sourceIncrement = (u32)-4;
       break;
     case 2:
       sourceIncrement = 0;
       break;
     }
     switch((DM0CNT_H >> 5) & 3) {
     case 0:
       break;
     case 1:
       destIncrement = (u32)-4;
       break;
     case 2:
       destIncrement = 0;
       break;
     }      
     u32 source = DM0SAD_L | (DM0SAD_H << 16);
     u32 dest = DM0DAD_L | (DM0DAD_H << 16);
#ifdef DEV_VERSION
     if(systemVerbose & VERBOSE_DMA0) {
       int count = (DM0CNT_L ? DM0CNT_L : 0x4000) << 1;
       if(DM0CNT_H & 0x0400)
         count <<= 1;
       log("DMA0: s=%08x d=%08x c=%04x count=%08x\n", source, dest, DM0CNT_H,
           count);
     }
#endif
     doDMA(source, dest, sourceIncrement, destIncrement,
           DM0CNT_L ? DM0CNT_L : 0x4000,
           DM0CNT_H & 0x0400);
     if(DM0CNT_H & 0x4000) {
       IF |= 0x0100;
       UPDATE_REG(0x202, IF);
     }
     
     if(((DM0CNT_H >> 5) & 3) != 3) {
       DM0DAD_L = dest & 0xFFFF;
       DM0DAD_H = dest >> 16;
       UPDATE_REG(0xB4, DM0DAD_L);
       UPDATE_REG(0xB6, DM0DAD_H);
     }
     DM0SAD_L = source & 0xFFFF;
     DM0SAD_H = source >> 16;
     UPDATE_REG(0xB0, DM0SAD_L);
     UPDATE_REG(0xB2, DM0SAD_H);      
     if(!(DM0CNT_H & 0x0200) || !(DM0CNT_H & 0x3000)) {
       DM0CNT_H &= 0x7FFF;
       UPDATE_REG(0xBA, DM0CNT_H);
       
       DM0DAD_L = dma0DestReload & 0xFFFF;
       DM0DAD_H = dma0DestReload >> 16;
       UPDATE_REG(0xB4, DM0DAD_L);
       UPDATE_REG(0xB6, DM0DAD_H);
       
       DM0SAD_L = dma0SourceReload & 0xFFFF;
       DM0SAD_H = dma0SourceReload >> 16;
       UPDATE_REG(0xB0, DM0SAD_L);
       UPDATE_REG(0xB2, DM0SAD_H);
     }
   }
  }
   
  // DMA 1
  if((DM1CNT_H & 0x8000) && (dmamask & 2)) {
    if(((DM1CNT_H >> 12) & 3) == reason) {
      u32 sourceIncrement = 4;
      u32 destIncrement = 4;
      switch((DM1CNT_H >> 7) & 3) {
      case 0:
        break;
      case 1:
        sourceIncrement = (u32)-4;
        break;
      case 2:
        sourceIncrement = 0;
        break;
      }
      switch((DM1CNT_H >> 5) & 3) {
      case 0:
        break;
      case 1:
        destIncrement = (u32)-4;
        break;
      case 2:
        destIncrement = 0;
        break;
      }      
      u32 source = DM1SAD_L | (DM1SAD_H << 16);
      u32 dest = DM1DAD_L | (DM1DAD_H << 16);
      if(reason == 3) {
#ifdef DEV_VERSION
        if(systemVerbose & VERBOSE_DMA1) {
          log("DMA1: s=%08x d=%08x c=%04x bytes=%08x\n", source, dest,
              DM1CNT_H,
              16);
        }
#endif  
        doDMA(source, dest, sourceIncrement, 0, 4,
              0x0400);
      } else {
#ifdef DEV_VERSION
        if(systemVerbose & VERBOSE_DMA1) {
          int count = (DM1CNT_L ? DM1CNT_L : 0x4000) << 1;
          if(DM1CNT_H & 0x0400)
            count <<= 1;
          log("DMA1: s=%08x d=%08x c=%04x bytes=%08x\n", source, dest,
              DM1CNT_H,
              count);
        }
#endif          
        doDMA(source, dest, sourceIncrement, destIncrement,
              DM1CNT_L ? DM1CNT_L : 0x4000,
              DM1CNT_H & 0x0400);
      }
        
      if(DM1CNT_H & 0x4000) {
        IF |= 0x0200;
        UPDATE_REG(0x202, IF);
      }
      
      if((((DM1CNT_H >> 5) & 3) != 3) && (reason != 3)) {
        DM1DAD_L = dest & 0xFFFF;
        DM1DAD_H = dest >> 16;
        UPDATE_REG(0xC0, DM1DAD_L);
        UPDATE_REG(0xC2, DM1DAD_H);
      }
      DM1SAD_L = source & 0xFFFF;
      DM1SAD_H = source >> 16;
      UPDATE_REG(0xBC, DM1SAD_L);
      UPDATE_REG(0xBE, DM1SAD_H);      
      if(!(DM1CNT_H & 0x0200) || !(DM1CNT_H & 0x3000)) {
        DM1CNT_H &= 0x7FFF;
        UPDATE_REG(0xC6, DM1CNT_H);
        
        DM1DAD_L = dma1DestReload & 0xFFFF;
        DM1DAD_H = dma1DestReload >> 16;
        UPDATE_REG(0xC0, DM1DAD_L);
        UPDATE_REG(0xC2, DM1DAD_H);
        
        DM1SAD_L = dma1SourceReload & 0xFFFF;
        DM1SAD_H = dma1SourceReload >> 16;
        UPDATE_REG(0xBC, DM1SAD_L);
        UPDATE_REG(0xBE, DM1SAD_H);
      }
    }
  }
  
  // DMA 2
  if((DM2CNT_H & 0x8000) && (dmamask & 4)) {
    if(((DM2CNT_H >> 12) & 3) == reason) {
      u32 sourceIncrement = 4;
      u32 destIncrement = 4;
      switch((DM2CNT_H >> 7) & 3) {
      case 0:
        break;
      case 1:
        sourceIncrement = (u32)-4;
        break;
      case 2:
        sourceIncrement = 0;
        break;
      }
      switch((DM2CNT_H >> 5) & 3) {
      case 0:
        break;
      case 1:
        destIncrement = (u32)-4;
        break;
      case 2:
        destIncrement = 0;
        break;
      }      
      u32 source = DM2SAD_L | (DM2SAD_H << 16);
      u32 dest = DM2DAD_L | (DM2DAD_H << 16);
      if(reason == 3) {
#ifdef DEV_VERSION
        if(systemVerbose & VERBOSE_DMA2) {
          int count = (4) << 2;
          log("DMA2: s=%08x d=%08x c=%04x bytes=%08x\n", source, dest,
              DM2CNT_H,
              count);
        }
#endif                  
        doDMA(source, dest, sourceIncrement, 0, 4,
              0x0400);
      } else {
#ifdef DEV_VERSION
        if(systemVerbose & VERBOSE_DMA2) {
          int count = (DM2CNT_L ? DM2CNT_L : 0x4000) << 1;
          if(DM2CNT_H & 0x0400)
            count <<= 1;
          log("DMA2: s=%08x d=%08x c=%04x bytes=%08x\n", source, dest,
              DM2CNT_H,
              count);
        }
#endif                  
        doDMA(source, dest, sourceIncrement, destIncrement,
              DM2CNT_L ? DM2CNT_L : 0x4000,
              DM2CNT_H & 0x0400);
      }
      if(DM2CNT_H & 0x4000) {
        IF |= 0x0400;
        UPDATE_REG(0x202, IF);
      }
      
      if((((DM2CNT_H >> 5) & 3) != 3) && (reason != 3)) {
        DM2DAD_L = dest & 0xFFFF;
        DM2DAD_H = dest >> 16;
        UPDATE_REG(0xCC, DM2DAD_L);
        UPDATE_REG(0xCE, DM2DAD_H);
      }
      DM2SAD_L = source & 0xFFFF;
      DM2SAD_H = source >> 16;
      UPDATE_REG(0xC8, DM2SAD_L);
      UPDATE_REG(0xCA, DM2SAD_H);      
      if(!(DM2CNT_H & 0x0200) || !(DM2CNT_H & 0x3000)) {
        DM2CNT_H &= 0x7FFF;
        UPDATE_REG(0xD2, DM2CNT_H);
       
        DM2DAD_L = dma2DestReload & 0xFFFF;
        DM2DAD_H = dma2DestReload >> 16;
        UPDATE_REG(0xCC, DM2DAD_L);
        UPDATE_REG(0xCE, DM2DAD_H);
        
        DM2SAD_L = dma2SourceReload & 0xFFFF;
        DM2SAD_H = dma2SourceReload >> 16;
        UPDATE_REG(0xC8, DM2SAD_L);
        UPDATE_REG(0xCA, DM2SAD_H);
      }
    }
  }

  // DMA 3
  if((DM3CNT_H & 0x8000) && (dmamask & 8)) {
    if(((DM3CNT_H >> 12) & 3) == reason) {
      u32 sourceIncrement = 4;
      u32 destIncrement = 4;
      switch((DM3CNT_H >> 7) & 3) {
      case 0:
        break;
      case 1:
        sourceIncrement = (u32)-4;
        break;
      case 2:
        sourceIncrement = 0;
        break;
      }
      switch((DM3CNT_H >> 5) & 3) {
      case 0:
        break;
      case 1:
        destIncrement = (u32)-4;
        break;
      case 2:
        destIncrement = 0;
        break;
      }      
      u32 source = DM3SAD_L | (DM3SAD_H << 16);
      u32 dest = DM3DAD_L | (DM3DAD_H << 16);
#ifdef DEV_VERSION
      if(systemVerbose & VERBOSE_DMA3) {
        int count = (DM3CNT_L ? DM3CNT_L : 0x10000) << 1;
        if(DM3CNT_H & 0x0400)
          count <<= 1;
        log("DMA3: s=%08x d=%08x c=%04x bytes=%08x\n", source, dest,
            DM3CNT_H,
            count);
      }
#endif                
      doDMA(source, dest, sourceIncrement, destIncrement,
            DM3CNT_L ? DM3CNT_L : 0x10000,
            DM3CNT_H & 0x0400);
      if(DM3CNT_H & 0x4000) {
        IF |= 0x0800;
        UPDATE_REG(0x202, IF);
      }
      
      if(((DM3CNT_H >> 5) & 3) != 3) {
        DM3DAD_L = dest & 0xFFFF;
        DM3DAD_H = dest >> 16;
        UPDATE_REG(0xD8, DM3DAD_L);
        UPDATE_REG(0xDA, DM3DAD_H);
      }
      DM3SAD_L = source & 0xFFFF;
      DM3SAD_H = source >> 16;
      UPDATE_REG(0xD4, DM3SAD_L);
      UPDATE_REG(0xD6, DM3SAD_H);      
      if(!(DM3CNT_H & 0x0200) || !(DM3CNT_H & 0x3000)) {
        DM3CNT_H &= 0x7FFF;
        UPDATE_REG(0xDE, DM3CNT_H);
       
        DM3DAD_L = dma3DestReload & 0xFFFF;
        DM3DAD_H = dma3DestReload >> 16;
        UPDATE_REG(0xD8, DM3DAD_L);
        UPDATE_REG(0xDA, DM3DAD_H);
        
        DM3SAD_L = dma3SourceReload & 0xFFFF;
        DM3SAD_H = dma3SourceReload >> 16;
        UPDATE_REG(0xD4, DM3SAD_L);
        UPDATE_REG(0xD6, DM3SAD_H);
      }
    }
  }
}

void CPUUpdateRegister(u32 address, u16 value)
{
  switch(address) {
  case 0x00:
    {
      bool change = ((DISPCNT ^ value) & 0x80) ? true : false;
      DISPCNT = (value & 0xFFF7);
      UPDATE_REG(0x00, DISPCNT);
      layerEnable = layerSettings & value;
      windowOn = (layerEnable & 0x6000) ? true : false;
      if(change && !((value & 0x80))) {
        if(!(DISPSTAT & 1)) {
          lcdTicks = 960;
          //      VCOUNT = 0;
          //      UPDATE_REG(0x06, VCOUNT);
          DISPSTAT &= 0xFFFC;
          UPDATE_REG(0x04, DISPSTAT);
          CPUCompareVCOUNT();
        }
        //        (*renderLine)();
      }
      CPUUpdateRender();
      //      CPUUpdateTicks();
    }
    break;
  case 0x04:
    DISPSTAT = (value & 0xFF38) | (DISPSTAT & 7);
    UPDATE_REG(0x04, DISPSTAT);
    break;
  case 0x06:
    // not writable
    break;
  case 0x08:
    BG0CNT = (value & 0xDFCF);
    UPDATE_REG(0x08, BG0CNT);
    break;
  case 0x0A:
    BG1CNT = (value & 0xDFCF);
    UPDATE_REG(0x0A, BG1CNT);
    break;
  case 0x0C:
    BG2CNT = (value & 0xFFCF);
    UPDATE_REG(0x0C, BG2CNT);
    break;
  case 0x0E:
    BG3CNT = (value & 0xFFCF);
    UPDATE_REG(0x0E, BG3CNT);
    break;
  case 0x10:
    BG0HOFS = value & 511;
    UPDATE_REG(0x10, BG0HOFS);
    break;
  case 0x12:
    BG0VOFS = value & 511;
    UPDATE_REG(0x12, BG0VOFS);
    break;
  case 0x14:
    BG1HOFS = value & 511;
    UPDATE_REG(0x14, BG1HOFS);
    break;
  case 0x16:
    BG1VOFS = value & 511;
    UPDATE_REG(0x16, BG1VOFS);
    break;      
  case 0x18:
    BG2HOFS = value & 511;
    UPDATE_REG(0x18, BG2HOFS);
    break;
  case 0x1A:
    BG2VOFS = value & 511;
    UPDATE_REG(0x1A, BG2VOFS);
    break;
  case 0x1C:
    BG3HOFS = value & 511;
    UPDATE_REG(0x1C, BG3HOFS);
    break;
  case 0x1E:
    BG3VOFS = value & 511;
    UPDATE_REG(0x1E, BG3VOFS);
    break;      
  case 0x20:
    BG2PA = value;
    UPDATE_REG(0x20, BG2PA);
    break;
  case 0x22:
    BG2PB = value;
    UPDATE_REG(0x22, BG2PB);
    break;
  case 0x24:
    BG2PC = value;
    UPDATE_REG(0x24, BG2PC);
    break;
  case 0x26:
    BG2PD = value;
    UPDATE_REG(0x26, BG2PD);
    break;
  case 0x28:
    BG2X_L = value;
    UPDATE_REG(0x28, BG2X_L);
    gfxBG2Changed |= 1;
    break;
  case 0x2A:
    BG2X_H = (value & 0xFFF);
    UPDATE_REG(0x2A, BG2X_H);
    gfxBG2Changed |= 1;    
    break;
  case 0x2C:
    BG2Y_L = value;
    UPDATE_REG(0x2C, BG2Y_L);
    gfxBG2Changed |= 2;    
    break;
  case 0x2E:
    BG2Y_H = value & 0xFFF;
    UPDATE_REG(0x2E, BG2Y_H);
    gfxBG2Changed |= 2;    
    break;
  case 0x30:
    BG3PA = value;
    UPDATE_REG(0x30, BG3PA);
    break;
  case 0x32:
    BG3PB = value;
    UPDATE_REG(0x32, BG3PB);
    break;
  case 0x34:
    BG3PC = value;
    UPDATE_REG(0x34, BG3PC);
    break;
  case 0x36:
    BG3PD = value;
    UPDATE_REG(0x36, BG3PD);
    break;
  case 0x38:
    BG3X_L = value;
    UPDATE_REG(0x38, BG3X_L);
    gfxBG3Changed |= 1;
    break;
  case 0x3A:
    BG3X_H = value & 0xFFF;
    UPDATE_REG(0x3A, BG3X_H);
    gfxBG3Changed |= 1;    
    break;
  case 0x3C:
    BG3Y_L = value;
    UPDATE_REG(0x3C, BG3Y_L);
    gfxBG3Changed |= 2;    
    break;
  case 0x3E:
    BG3Y_H = value & 0xFFF;
    UPDATE_REG(0x3E, BG3Y_H);
    gfxBG3Changed |= 2;    
    break;
  case 0x40:
    WIN0H = value;
    UPDATE_REG(0x40, WIN0H);
    CPUUpdateWindow0();
    break;
  case 0x42:
    WIN1H = value;
    UPDATE_REG(0x42, WIN1H);
    CPUUpdateWindow1();    
    break;      
  case 0x44:
    WIN0V = value;
    UPDATE_REG(0x44, WIN0V);
    break;
  case 0x46:
    WIN1V = value;
    UPDATE_REG(0x46, WIN1V);
    break;
  case 0x48:
    WININ = value & 0x3F3F;
    UPDATE_REG(0x48, WININ);
    break;
  case 0x4A:
    WINOUT = value & 0x3F3F;
    UPDATE_REG(0x4A, WINOUT);
    break;
  case 0x4C:
    MOSAIC = value;
    UPDATE_REG(0x4C, MOSAIC);
    break;
  case 0x50:
    BLDMOD = value & 0x3FFF;
    UPDATE_REG(0x50, BLDMOD);
    fxOn = ((BLDMOD>>6)&3) != 0;
    CPUUpdateRender();
    break;
  case 0x52:
    COLEV = value & 0x1F1F;
    UPDATE_REG(0x52, COLEV);
    break;
  case 0x54:
    COLY = value & 0x1F;
    UPDATE_REG(0x54, COLY);
    break;
  case 0x60:
  case 0x62:
  case 0x64:
  case 0x68:
  case 0x6c:
  case 0x70:
  case 0x72:
  case 0x74:
  case 0x78:
  case 0x7c:
  case 0x80:
  case 0x84:
    soundEvent(address&0xFF, (u8)(value & 0xFF));
    soundEvent((address&0xFF)+1, (u8)(value>>8));
    break;
  case 0x82:
  case 0x88:
  case 0xa0:
  case 0xa2:
  case 0xa4:
  case 0xa6:
  case 0x90:
  case 0x92:
  case 0x94:
  case 0x96:
  case 0x98:
  case 0x9a:
  case 0x9c:
  case 0x9e:    
    soundEvent(address&0xFF, value);
    break;
  case 0xB0:
    DM0SAD_L = value;
    UPDATE_REG(0xB0, DM0SAD_L);
    dma0SourceReload = (dma0SourceReload & 0xFFFF0000) | value;
    break;
  case 0xB2:
    DM0SAD_H = value & 0x07FF;
    UPDATE_REG(0xB2, DM0SAD_H);
    dma0SourceReload = (dma0SourceReload & 0xFFFF) | (value<<16);      
    break;
  case 0xB4:
    DM0DAD_L = value;
    UPDATE_REG(0xB4, DM0DAD_L);
    dma0DestReload = (dma0DestReload & 0xFFFF0000) | value;
    break;
  case 0xB6:
    DM0DAD_H = value & 0x07FF;
    UPDATE_REG(0xB6, DM0DAD_H);
    dma0DestReload = (dma0DestReload & 0xFFFF) | (value<<16);
    break;
  case 0xB8:
    DM0CNT_L = value & 0x3FFF;
    UPDATE_REG(0xB8, 0);
    break;
  case 0xBA:
    value &= 0xF7E0;
    
    if((DM0CNT_H & 0x8200) == 0x8200 && (value & 0x8200) == 0x8000) {
      if(DM0CNT_H & 0x3000) {
        DM0CNT_H = value;
        UPDATE_REG(0xBA, DM0CNT_H);
        break;
      }
    }    
    
    if(DM0CNT_H & 0x8000) {
      if(!(value & 0x8000)) {
        DM0DAD_L = dma0DestReload & 0xFFFF;
        DM0DAD_H = dma0DestReload >> 16;
        UPDATE_REG(0xB4, DM0DAD_L);
        UPDATE_REG(0xB6, DM0DAD_H);
        
        DM0SAD_L = dma0SourceReload & 0xFFFF;
        DM0SAD_H = dma0SourceReload >> 16;
        UPDATE_REG(0xB0, DM0SAD_L);
        UPDATE_REG(0xB2, DM0SAD_H);                       
      }
    }
    DM0CNT_H = value;
    UPDATE_REG(0xBA, DM0CNT_H);    
    CPUCheckDMA(0, 1);
    break;      
  case 0xBC:
    DM1SAD_L = value;
    UPDATE_REG(0xBC, DM1SAD_L);
    dma1SourceReload = (dma1SourceReload & 0xFFFF0000) | value;
    break;
  case 0xBE:
    DM1SAD_H = value & 0x0FFF;
    UPDATE_REG(0xBE, DM1SAD_H);
    dma1SourceReload = (dma1SourceReload & 0xFFFF) | (value << 16);
    break;
  case 0xC0:
    DM1DAD_L = value;
    UPDATE_REG(0xC0, DM1DAD_L);
    dma1DestReload = (dma1DestReload & 0xFFFF0000) | value;
    break;
  case 0xC2:
    DM1DAD_H = value & 0x07FF;
    UPDATE_REG(0xC2, DM1DAD_H);
    dma1DestReload = (dma1DestReload & 0xFFFF) | (value << 16);
    break;
  case 0xC4:
    DM1CNT_L = value & 0x3FFF;
    UPDATE_REG(0xC4, 0);
    break;
  case 0xC6:
    value &= 0xF7E0;
    
    if((DM1CNT_H & 0x8200) == 0x8200 && (value & 0x8200) == 0x8000) {
      if(DM1CNT_H & 0x3000) {
        DM1CNT_H = value;
        UPDATE_REG(0xC6, DM1CNT_H);
        break;
      }
    }
    
    if(DM1CNT_H & 0x8000) {
      if(!(value & 0x8000)) {
        DM1DAD_L = dma1DestReload & 0xFFFF;
        DM1DAD_H = dma1DestReload >> 16;
        UPDATE_REG(0xC0, DM1DAD_L);
        UPDATE_REG(0xC2, DM1DAD_H);
        
        DM1SAD_L = dma1SourceReload & 0xFFFF;
        DM1SAD_H = dma1SourceReload >> 16;
        UPDATE_REG(0xBC, DM1SAD_L);
        UPDATE_REG(0xBE, DM1SAD_H);                       
      }
    }
    DM1CNT_H = value;
    UPDATE_REG(0xC6, DM1CNT_H);
    
    CPUCheckDMA(0, 2);
    break;
  case 0xC8:
    DM2SAD_L = value;
    UPDATE_REG(0xC8, DM2SAD_L);
    dma2SourceReload = (dma2SourceReload & 0xFFFF0000) | value;
    break;
  case 0xCA:
    DM2SAD_H = value & 0x0FFF;
    UPDATE_REG(0xCA, DM2SAD_H);
    dma2SourceReload = (dma2SourceReload & 0xFFFF) | (value<<16);
    break;
  case 0xCC:
    DM2DAD_L = value;
    UPDATE_REG(0xCC, DM2DAD_L);
    dma2DestReload = (dma2DestReload & 0xFFFF0000) | value;
    break;
  case 0xCE:
    DM2DAD_H = value & 0x07FF;
    UPDATE_REG(0xCE, DM2DAD_H);
    dma2DestReload = (dma2DestReload & 0xFFFF) | (value<<16);
    break;
  case 0xD0:
    DM2CNT_L = value & 0x3FFF;
    UPDATE_REG(0xD0, 0);
    break;
  case 0xD2:
    value &= 0xF7E0;
    
    if((DM2CNT_H & 0x8200) == 0x8200 && (value & 0x8200) == 0x8000) {
      if(DM2CNT_H & 0x3000) {
        DM2CNT_H = value;
        UPDATE_REG(0xD2, DM2CNT_H);
        break;
      }
    }
    
    if(DM2CNT_H & 0x8000) {
      if(!(value & 0x8000)) {
        DM2DAD_L = dma2DestReload & 0xFFFF;
        DM2DAD_H = dma2DestReload >> 16;
        UPDATE_REG(0xCC, DM2DAD_L);
        UPDATE_REG(0xCE, DM2DAD_H);
        
        DM2SAD_L = dma2SourceReload & 0xFFFF;
        DM2SAD_H = dma2SourceReload >> 16;
        UPDATE_REG(0xC8, DM2SAD_L);
        UPDATE_REG(0xCA, DM2SAD_H);                       
      }
    }            
    DM2CNT_H = value;
    UPDATE_REG(0xD2, DM2CNT_H);
    
    CPUCheckDMA(0, 4);
    break;
  case 0xD4:
    DM3SAD_L = value;
    UPDATE_REG(0xD4, DM3SAD_L);
    dma3SourceReload = (dma3SourceReload & 0xFFFF0000) | value;
    break;
  case 0xD6:
    DM3SAD_H = value & 0x0FFF;
    UPDATE_REG(0xD6, DM3SAD_H);
    dma3SourceReload = (dma3SourceReload & 0xFFFF) | (value<<16);
    break;
  case 0xD8:
    DM3DAD_L = value;
    UPDATE_REG(0xD8, DM3DAD_L);
    dma3DestReload = (dma3DestReload & 0xFFFF0000) | value;
    break;
  case 0xDA:
    DM3DAD_H = value & 0x0FFF;
    UPDATE_REG(0xDA, DM3DAD_H);
    dma3DestReload = (dma3DestReload & 0xFFFF) | (value<<16);
    break;
  case 0xDC:
    DM3CNT_L = value;
    UPDATE_REG(0xDC, 0);
    break;
  case 0xDE:
    value &= 0xFFE0;
    
    if((DM3CNT_H & 0x8200) == 0x8200 && (value & 0x8200) == 0x8000) {
      if(DM3CNT_H & 0x3000) {
        DM3CNT_H = value;
        UPDATE_REG(0xDE, DM3CNT_H);
        break;
      }
    }
    
    if(DM3CNT_H & 0x8000) {
      if(!(value & 0x8000)) {
        DM3DAD_L = dma3DestReload & 0xFFFF;
        DM3DAD_H = dma3DestReload >> 16;
        UPDATE_REG(0xD8, DM3DAD_L);
        UPDATE_REG(0xDA, DM3DAD_H);
        
        DM3SAD_L = dma3SourceReload & 0xFFFF;
        DM3SAD_H = dma3SourceReload >> 16;
        UPDATE_REG(0xD4, DM3SAD_L);
        UPDATE_REG(0xD6, DM3SAD_H);                       
      }
    }
    DM3CNT_H = value;
    UPDATE_REG(0xDE, DM3CNT_H);
    
    CPUCheckDMA(0,8);
    break;
  case 0x100:
    timer0Reload = value;
    break;
  case 0x102:
    timer0Ticks = timer0ClockReload = TIMER_TICKS[value & 3];        
    if(!timer0On && (value & 0x80)) {
      // reload the counter
      TM0D = timer0Reload;      
      if(timer0ClockReload == 1)
        timer0Ticks = 0x10000 - TM0D;
      UPDATE_REG(0x100, TM0D);
    }
    timer0On = value & 0x80 ? true : false;
    TM0CNT = value & 0xC7;
    UPDATE_REG(0x102, TM0CNT);
    //    CPUUpdateTicks();
    break;
  case 0x104:
    timer1Reload = value;
    break;
  case 0x106:
    timer1Ticks = timer1ClockReload = TIMER_TICKS[value & 3];        
    if(!timer1On && (value & 0x80)) {
      // reload the counter
      TM1D = timer1Reload;      
      if(timer1ClockReload == 1)
        timer1Ticks = 0x10000 - TM1D;
      UPDATE_REG(0x104, TM1D);
    }
    timer1On = value & 0x80 ? true : false;
    TM1CNT = value & 0xC7;
    UPDATE_REG(0x106, TM1CNT);
    break;
  case 0x108:
    timer2Reload = value;
    break;
  case 0x10A:
    timer2Ticks = timer2ClockReload = TIMER_TICKS[value & 3];        
    if(!timer2On && (value & 0x80)) {
      // reload the counter
      TM2D = timer2Reload;      
      if(timer2ClockReload == 1)
        timer2Ticks = 0x10000 - TM2D;
      UPDATE_REG(0x108, TM2D);
    }
    timer2On = value & 0x80 ? true : false;
    TM2CNT = value & 0xC7;
    UPDATE_REG(0x10A, TM2CNT);
    break;
  case 0x10C:
    timer3Reload = value;
    break;
  case 0x10E:
    timer3Ticks = timer3ClockReload = TIMER_TICKS[value & 3];        
    if(!timer3On && (value & 0x80)) {
      // reload the counter
      TM3D = timer3Reload;      
      if(timer3ClockReload == 1)
        timer3Ticks = 0x10000 - TM3D;
      UPDATE_REG(0x10C, TM3D);
    }
    timer3On = value & 0x80 ? true : false;
    TM3CNT = value & 0xC7;
    UPDATE_REG(0x10E, TM3CNT);
    break;
  case 0x128:
    if(value & 0x80) {
      value &= 0xff7f;
      if(value & 1 && (value & 0x4000)) {
        UPDATE_REG(0x12a, 0xFF);
        IF |= 0x80;
        UPDATE_REG(0x202, IF);
        value &= 0x7f7f;
      }
    }
    UPDATE_REG(0x128, value);
    break;
  case 0x130:
    P1 |= (value & 0x3FF);
    UPDATE_REG(0x130, P1);
    break;
  case 0x132:
    UPDATE_REG(0x132, value & 0xC3FF);
    break;
  case 0x200:
    IE = value & 0x3FFF;
    UPDATE_REG(0x200, IE);
    if((IME & 1) && (IF & IE) && armIrqEnable) {
      CPU_BREAK_LOOP_2;
    }    
    break;
  case 0x202:
    IF ^= (value & IF);
    UPDATE_REG(0x202, IF);
    break;
  case 0x204:
    memoryWait[0x0e] = memoryWaitSeq[0x0e] = gamepakRamWaitState[value & 3];

    if(!speedHack) {
      memoryWait[0x08] = memoryWait[0x09] = gamepakWaitState[(value >> 2) & 7];
      memoryWaitSeq[0x08] = memoryWaitSeq[0x09] =
        gamepakWaitState0[(value >> 2) & 7];
      
      memoryWait[0x0a] = memoryWait[0x0b] = gamepakWaitState[(value >> 5) & 7];
      memoryWaitSeq[0x0a] = memoryWaitSeq[0x0b] =
        gamepakWaitState1[(value >> 5) & 7];
      
      memoryWait[0x0c] = memoryWait[0x0d] = gamepakWaitState[(value >> 8) & 7];
      memoryWaitSeq[0x0c] = memoryWaitSeq[0x0d] =
        gamepakWaitState2[(value >> 8) & 7];
    } else {
      memoryWait[0x08] = memoryWait[0x09] = 4;
      memoryWaitSeq[0x08] = memoryWaitSeq[0x09] = 2;

      memoryWait[0x0a] = memoryWait[0x0b] = 4;
      memoryWaitSeq[0x0a] = memoryWaitSeq[0x0b] = 4;

      memoryWait[0x0c] = memoryWait[0x0d] = 4;
      memoryWaitSeq[0x0c] = memoryWaitSeq[0x0d] = 8;
    }
    {
      for(int i = 0; i < 16; i++) {
        memoryWaitFetch32[i] = memoryWait32[i] = memoryWait[i] *
          (memory32[i] ? 1 : 2);
        memoryWaitFetch[i] = memoryWait[i];
      }
    }
    if(value & 0x4000) {
      for(int i = 0; i < 16; i++) {
        memoryWaitFetch32[i] = memoryWaitFetch[i] = cpuMemoryWait[i];
      }
    }
    UPDATE_REG(0x204, value);
    break;
  case 0x208:
    IME = value & 1;
    UPDATE_REG(0x208, IME);
    if((IME & 1) && (IF & IE) && armIrqEnable) {
      CPU_BREAK_LOOP_2;
    }
    break;
  case 0x300:
    if(value != 0)
      value &= 0xFFFE;
    UPDATE_REG(0x300, value);
    break;
  default:
    UPDATE_REG(address&0x3FE, value);
    break;
  }
}

void CPUWriteHalfWord(u32 address, u16 value)
{
#ifdef DEV_VERSION
  if(address & 1) {
    if(systemVerbose & VERBOSE_UNALIGNED_MEMORY) {
      log("Unaligned halfword write: %04x to %08x from %08x\n",
          value,
          address,
          armMode ? armNextPC - 4 : armNextPC - 2);
    }
  }
#endif
  
  switch(address >> 24) {
  case 2:
#ifdef SDL
    if(*((u16 *)&freezeWorkRAM[address & 0x3FFFE]))
      cheatsWriteHalfWord((u16 *)&workRAM[address & 0x3FFFE],
                          value,
                          *((u16 *)&freezeWorkRAM[address & 0x3FFFE]));
    else
#endif
      *((u16 *)&workRAM[address & 0x3FFFE]) = TO16LE(value);
    break;
  case 3:
#ifdef SDL
    if(*((u16 *)&freezeInternalRAM[address & 0x7ffe]))
      cheatsWriteHalfWord((u16 *)&internalRAM[address & 0x7ffe],
                          value,
                          *((u16 *)&freezeInternalRAM[address & 0x7ffe]));
    else
#endif
      *((u16 *)&internalRAM[address & 0x7ffe]) = TO16LE(value);
    break;    
  case 4:
    CPUUpdateRegister(address & 0x3fe, value);
    break;
  case 5:
    *((u16 *)&paletteRAM[address & 0x3fe]) = TO16LE(value);
    break;
  case 6:
    if(address & 0x10000)
      *((u16 *)&vram[address & 0x17ffe]) = TO16LE(value);
    else
      *((u16 *)&vram[address & 0x1fffe]) = TO16LE(value);
    break;
  case 7:
    *((u16 *)&oam[address & 0x3fe]) = TO16LE(value);
    break;
  case 13:
    if(cpuEEPROMEnabled) {
      eepromWrite(address, (u8)value);
      break;
    }
    goto unwritable;
  case 14:
    if(!eepromInUse | cpuSramEnabled | cpuFlashEnabled) {
      (*cpuSaveGameFunc)(address, (u8)value);
      break;
    }
    goto unwritable;
  default:
  unwritable:
#ifdef DEV_VERSION
    if(systemVerbose & VERBOSE_ILLEGAL_WRITE) {
      log("Illegal halfword write: %04x to %08x from %08x\n",
          value,
          address,
          armMode ? armNextPC - 4 : armNextPC - 2);
    }
#endif
    break;
  }
}

void CPUWriteByte(u32 address, u8 b)
{
  switch(address >> 24) {
  case 2:
#ifdef SDL
      if(freezeWorkRAM[address & 0x3FFFF])
        cheatsWriteByte(&workRAM[address & 0x3FFFF], b);
      else
#endif  
        workRAM[address & 0x3FFFF] = b;
    break;
  case 3:
#ifdef SDL
    if(freezeInternalRAM[address & 0x7fff])
      cheatsWriteByte(&internalRAM[address & 0x7fff], b);
    else
#endif
      internalRAM[address & 0x7fff] = b;
    break;
  case 4:
    switch(address & 0x3FF) {
    case 0x301:
      if(b == 0x80)
        stopState = true;
      holdState = 1;
      holdType = -1;
      break;
    case 0x60:
    case 0x61:
    case 0x62:
    case 0x63:
    case 0x64:
    case 0x65:
    case 0x68:
    case 0x69:
    case 0x6c:
    case 0x6d:
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x78:
    case 0x79:
    case 0x7c:
    case 0x7d:
    case 0x80:
    case 0x81:
    case 0x84:
    case 0x85:
    case 0x90:
    case 0x91:
    case 0x92:
    case 0x93:
    case 0x94:
    case 0x95:
    case 0x96:
    case 0x97:
    case 0x98:
    case 0x99:
    case 0x9a:
    case 0x9b:
    case 0x9c:
    case 0x9d:
    case 0x9e:
    case 0x9f:      
      soundEvent(address&0xFF, b);
      break;
    default:
      //      if(address & 1) {
      //        CPUWriteHalfWord(address-1, (CPUReadHalfWord(address-1)&0x00FF)|((int)b<<8));
      //      } else
      if(address & 1)
        CPUUpdateRegister(address & 0x3fe,
                          ((FROM16LE(*((u16 *)&ioMem[address & 0x3fe])))
                           & 0x00FF) |
                          b<<8);
      else
        CPUUpdateRegister(address & 0x3fe,
                          ((FROM16LE(*((u16 *)&ioMem[address & 0x3fe])) & 0xFF00) | b));
    }
    break;
  case 5:
    // no need to switch
    *((u16 *)&paletteRAM[address & 0x3FE]) = (b << 8) | b;
    break;
  case 6:
    // no need to switch
    if(address & 0x10000)
      *((u16 *)&vram[address & 0x17FFE]) = (b << 8) | b;
    else
      *((u16 *)&vram[address & 0x1FFFE]) = (b << 8) | b;
    break;
  case 7:
    // no need to switch
    *((u16 *)&oam[address & 0x3FE]) = (b << 8) | b;
    break;    
  case 13:
    if(cpuEEPROMEnabled) {
      eepromWrite(address, b);
      break;
    }
    goto unwritable;
  case 14:
    if(!eepromInUse | cpuSramEnabled | cpuFlashEnabled) {
      (*cpuSaveGameFunc)(address, b);
      break;
    }
    // default
  default:
  unwritable:
#ifdef DEV_VERSION
    if(systemVerbose & VERBOSE_ILLEGAL_WRITE) {
      log("Illegal byte write: %02x to %08x from %08x\n",
          b,
          address,
          armMode ? armNextPC - 4 : armNextPC -2 );
    }
#endif
    break;
  }
}

u8 cpuBitsSet[256];
u8 cpuLowestBitSet[256];

void CPUInit(char *biosFileName, bool useBiosFile)
{
#ifdef WORDS_BIGENDIAN
  if(!cpuBiosSwapped) {
    for(int i = 0; i < sizeof(myROM)/4; i++) {
      myROM[i] = TO32LE(myROM[i]);
    }
    cpuBiosSwapped = true;
  }
#endif
  gbaSaveType = 0;
  eepromInUse = 0;
  saveType = 0;
  useBios = false;
  
  if(useBiosFile) {
    FILE *f = fopen(biosFileName, "rb");

    if(f != NULL) {
      size_t s = fread(bios, 1, 0x4000, f);
      if(s == 0x4000)
        useBios = true;
      else
        systemMessage(MSG_INVALID_BIOS_FILE_SIZE, "Invalid BIOS file size");
      
      fclose(f);
    }
  }
  
  if(!useBios) {
    memcpy(bios, myROM, sizeof(myROM));
  }

  int i = 0;

  biosProtected[0] = 0x00;
  biosProtected[1] = 0xf0;
  biosProtected[2] = 0x29;
  biosProtected[3] = 0xe1;

  for(i = 0; i < 256; i++) {
    int count = 0;
    int j;
    for(j = 0; j < 8; j++)
      if(i & (1 << j))
        count++;
    cpuBitsSet[i] = count;
    
    for(j = 0; j < 8; j++)
      if(i & (1 << j))
        break;
    cpuLowestBitSet[i] = j;
  }

  for(i = 0; i < 0x400; i++)
    ioReadable[i] = true;
  for(i = 0x10; i < 0x48; i++)
    ioReadable[i] = false;
  for(i = 0x4c; i < 0x50; i++)
    ioReadable[i] = false;
  for(i = 0x54; i < 0x60; i++)
    ioReadable[i] = false;
  for(i = 0x8c; i < 0x90; i++)
    ioReadable[i] = false;
  for(i = 0xa0; i < 0xb8; i++)
    ioReadable[i] = false;
  for(i = 0xbc; i < 0xc4; i++)
    ioReadable[i] = false;
  for(i = 0xc8; i < 0xd0; i++)
    ioReadable[i] = false;
  for(i = 0xd4; i < 0xdc; i++)
    ioReadable[i] = false;
  for(i = 0xe0; i < 0x100; i++)
    ioReadable[i] = false;
  for(i = 0x110; i < 0x120; i++)
    ioReadable[i] = false;
  for(i = 0x12c; i < 0x130; i++)
    ioReadable[i] = false;
  for(i = 0x138; i < 0x140; i++)
    ioReadable[i] = false;
  for(i = 0x144; i < 0x150; i++)
    ioReadable[i] = false;
  for(i = 0x15c; i < 0x200; i++)
    ioReadable[i] = false;
  for(i = 0x20c; i < 0x300; i++)
    ioReadable[i] = false;
  for(i = 0x304; i < 0x400; i++)
    ioReadable[i] = false;
}

void CPUReset()
{
  if(gbaSaveType == 0) {
    if(eepromInUse)
      gbaSaveType = 3;
    else
      switch(saveType) {
      case 1:
        gbaSaveType = 1;
        break;
      case 2:
        gbaSaveType = 2;
        break;
      }
  }
  // clen registers
  memset(&reg[0], 0, sizeof(reg));
  // clean OAM
  memset(oam, 0, 0x400);
  // clean palette
  memset(paletteRAM, 0, 0x400);
  // clean picture
  memset(pix, 0, 4*160*240);
  // clean vram
  memset(vram, 0, 0x20000);
  // clean io memory
  memset(ioMem, 0, 0x400);

  DISPCNT  = 0x0080;
  DISPSTAT = 0x0000;
  VCOUNT   = 0x0000;
  BG0CNT   = 0x0000;
  BG1CNT   = 0x0000;
  BG2CNT   = 0x0000;
  BG3CNT   = 0x0000;
  BG0HOFS  = 0x0000;
  BG0VOFS  = 0x0000;
  BG1HOFS  = 0x0000;
  BG1VOFS  = 0x0000;
  BG2HOFS  = 0x0000;
  BG2VOFS  = 0x0000;
  BG3HOFS  = 0x0000;
  BG3VOFS  = 0x0000;
  BG2PA    = 0x0100;
  BG2PB    = 0x0000;
  BG2PC    = 0x0000;
  BG2PD    = 0x0100;
  BG2X_L   = 0x0000;
  BG2X_H   = 0x0000;
  BG2Y_L   = 0x0000;
  BG2Y_H   = 0x0000;
  BG3PA    = 0x0100;
  BG3PB    = 0x0000;
  BG3PC    = 0x0000;
  BG3PD    = 0x0100;
  BG3X_L   = 0x0000;
  BG3X_H   = 0x0000;
  BG3Y_L   = 0x0000;
  BG3Y_H   = 0x0000;
  WIN0H    = 0x0000;
  WIN1H    = 0x0000;
  WIN0V    = 0x0000;
  WIN1V    = 0x0000;
  WININ    = 0x0000;
  WINOUT   = 0x0000;
  MOSAIC   = 0x0000;
  BLDMOD   = 0x0000;
  COLEV    = 0x0000;
  COLY     = 0x0000;
  DM0SAD_L = 0x0000;
  DM0SAD_H = 0x0000;
  DM0DAD_L = 0x0000;
  DM0DAD_H = 0x0000;
  DM0CNT_L = 0x0000;
  DM0CNT_H = 0x0000;
  DM1SAD_L = 0x0000;
  DM1SAD_H = 0x0000;
  DM1DAD_L = 0x0000;
  DM1DAD_H = 0x0000;
  DM1CNT_L = 0x0000;
  DM1CNT_H = 0x0000;
  DM2SAD_L = 0x0000;
  DM2SAD_H = 0x0000;
  DM2DAD_L = 0x0000;
  DM2DAD_H = 0x0000;
  DM2CNT_L = 0x0000;
  DM2CNT_H = 0x0000;
  DM3SAD_L = 0x0000;
  DM3SAD_H = 0x0000;
  DM3DAD_L = 0x0000;
  DM3DAD_H = 0x0000;
  DM3CNT_L = 0x0000;
  DM3CNT_H = 0x0000;
  TM0D     = 0x0000;
  TM0CNT   = 0x0000;
  TM1D     = 0x0000;
  TM1CNT   = 0x0000;
  TM2D     = 0x0000;
  TM2CNT   = 0x0000;
  TM3D     = 0x0000;
  TM3CNT   = 0x0000;
  P1       = 0x03FF;
  IE       = 0x0000;
  IF       = 0x0000;
  IME      = 0x0000;

  armMode = 0x1F;
  
  if(cpuIsMultiBoot) {
    reg[13].I = 0x03007F00;
    reg[15].I = 0x02000000;
    reg[16].I = 0x00000000;
    reg[R13_IRQ].I = 0x03007FA0;
    reg[R13_SVC].I = 0x03007FE0;
    armIrqEnable = true;
  } else {
    if(useBios) {
      reg[15].I = 0x00000000;
      armMode = 0x13;
      armIrqEnable = false;      
    } else {
      reg[13].I = 0x03007F00;
      reg[15].I = 0x08000000;
      reg[16].I = 0x00000000;
      reg[R13_IRQ].I = 0x03007FA0;
      reg[R13_SVC].I = 0x03007FE0;
      armIrqEnable = true;      
    }    
  }
  armState = true;
  C_FLAG = V_FLAG = N_FLAG = Z_FLAG = false;
  UPDATE_REG(0x00, DISPCNT);
  UPDATE_REG(0x20, BG2PA);
  UPDATE_REG(0x26, BG2PD);
  UPDATE_REG(0x30, BG3PA);
  UPDATE_REG(0x36, BG3PD);
  UPDATE_REG(0x130, P1);
  UPDATE_REG(0x88, 0x200);

  // disable FIQ
  reg[16].I |= 0x40;
  
  CPUUpdateCPSR();
  
  armNextPC = reg[15].I;
  reg[15].I += 4;

  // reset internal state
  holdState = false;
  holdType = 0;
  
  biosProtected[0] = 0x00;
  biosProtected[1] = 0xf0;
  biosProtected[2] = 0x29;
  biosProtected[3] = 0xe1;
  
  lcdTicks = 960;
  timer0On = false;
  timer0Ticks = 0;
  timer0Reload = 0;
  timer0ClockReload  = 0;
  timer1On = false;
  timer1Ticks = 0;
  timer1Reload = 0;
  timer1ClockReload  = 0;
  timer2On = false;
  timer2Ticks = 0;
  timer2Reload = 0;
  timer2ClockReload  = 0;
  timer3On = false;
  timer3Ticks = 0;
  timer3Reload = 0;
  timer3ClockReload  = 0;
  dma0SourceReload = 0;
  dma0DestReload = 0;
  dma1SourceReload = 0;
  dma1DestReload = 0;
  dma2SourceReload = 0;
  dma2DestReload = 0;
  dma3SourceReload = 0;
  dma3DestReload = 0;
  cpuSaveGameFunc = flashSaveDecide;
  renderLine = mode0RenderLine;
  fxOn = false;
  windowOn = false;
  frameCount = 0;
  saveType = 0;
  layerEnable = DISPCNT & layerSettings;
  
  for(int i = 0; i < 256; i++) {
    map[i].address = (u8 *)&dummyAddress;
    map[i].mask = 0;
  }

  map[0].address = bios;
  map[0].mask = 0x3FFF;
  map[2].address = workRAM;
  map[2].mask = 0x3FFFF;
  map[3].address = internalRAM;
  map[3].mask = 0x7FFF;
  map[4].address = ioMem;
  map[4].mask = 0x3FF;
  map[5].address = paletteRAM;
  map[5].mask = 0x3FF;
  map[6].address = vram;
  map[6].mask = 0x1FFFF;
  map[7].address = oam;
  map[7].mask = 0x3FF;
  map[8].address = rom;
  map[8].mask = 0x1FFFFFF;
  map[10].address = rom;
  map[10].mask = 0x1FFFFFF;
  map[12].address = rom;
  map[12].mask = 0x1FFFFFF;
  map[14].address = flashSaveMemory;
  map[14].mask = 0xFFFF;

  eepromReset();
  flashReset();
  
  soundReset();

  CPUUpdateWindow0();
  CPUUpdateWindow1();

  // make sure registers are correctly initialized if not using BIOS
  if(!useBios) {
    if(cpuIsMultiBoot)
      BIOS_RegisterRamReset(0xfe);
    else
      BIOS_RegisterRamReset(0xff);
  } else {
    if(cpuIsMultiBoot)
      BIOS_RegisterRamReset(0xfe);
  }

  switch(cpuSaveType) {
  case 0: // automatic
    cpuSramEnabled = true;
    cpuFlashEnabled = true;
    cpuEEPROMEnabled = true;
    cpuEEPROMSensorEnabled = false;
    break;
  case 1: // EEPROM
    cpuSramEnabled = false;
    cpuFlashEnabled = false;
    cpuEEPROMEnabled = true;
    cpuEEPROMSensorEnabled = false;
    break;
  case 2: // SRAM
    cpuSramEnabled = true;
    cpuFlashEnabled = false;
    cpuEEPROMEnabled = false;
    cpuEEPROMSensorEnabled = false;
    cpuSaveGameFunc = sramWrite;
    break;
  case 3: // FLASH
    cpuSramEnabled = false;
    cpuFlashEnabled = true;
    cpuEEPROMEnabled = false;
    cpuEEPROMSensorEnabled = false;
    cpuSaveGameFunc = flashWrite;
    break;
  case 4: // EEPROM+Sensor
    cpuSramEnabled = false;
    cpuFlashEnabled = false;
    cpuEEPROMEnabled = true;
    cpuEEPROMSensorEnabled = true;
    break;
  } 
  
  lastTime = systemGetClock();
}

void CPUInterrupt()
{
  u32 PC = reg[15].I;
  bool savedState = armState;
  CPUSwitchMode(0x12, true, false);
  reg[14].I = PC;
  if(!savedState)
    reg[14].I += 2;
  reg[15].I = 0x18;
  armState = true;
  armIrqEnable = false;

  armNextPC = reg[15].I;
  reg[15].I += 4;

  //  if(!holdState)
  biosProtected[0] = 0x02;
  biosProtected[1] = 0xc0;
  biosProtected[2] = 0x5e;
  biosProtected[3] = 0xe5;
}

#ifdef SDL
void log(char *defaultMsg, ...)
{
  char buffer[2048];
  va_list valist;
  
  va_start(valist, defaultMsg);
  vsprintf(buffer, defaultMsg, valist);

  if(out == NULL) {
    out = fopen("trace.log","w");
  }

  fputs(buffer, out);
  
  va_end(valist);
}
#endif

void CPULoop(int ticks)
{  
  int clockTicks;
  int cpuLoopTicks = 0;
  int timerOverflow = 0;
  // variables used by the CPU core

  extCpuLoopTicks = &cpuLoopTicks;
  extClockTicks = &clockTicks;
  extTicks = &ticks;

  CPUUpdateTicks(cpuLoopTicks);
  if(cpuLoopTicks > ticks) {
    cpuLoopTicks = ticks;
    cpuSavedTicks = ticks;
  }

  if(intState) {
    cpuLoopTicks = 5;
    cpuSavedTicks = 5;
  }
  
  for(;;) {
#ifndef FINAL_VERSION
    if(systemDebug) {
      if(systemDebug >= 10 && !holdState) {
        CPUUpdateCPSR();
        sprintf(buffer, "R00=%08x R01=%08x R02=%08x R03=%08x R04=%08x R05=%08x R06=%08x R07=%08x R08=%08x R09=%08x R10=%08x R11=%08x R12=%08x R13=%08x R14=%08x R15=%08x R16=%08x R17=%08x\n",
                 reg[0].I, reg[1].I, reg[2].I, reg[3].I, reg[4].I, reg[5].I,
                 reg[6].I, reg[7].I, reg[8].I, reg[9].I, reg[10].I, reg[11].I,
                 reg[12].I, reg[13].I, reg[14].I, reg[15].I, reg[16].I,
                 reg[17].I);
        log(buffer);
      } else if(!holdState) {
        sprintf(buffer, "PC=%08x\n", armNextPC);
        log(buffer);
      }
    }
#endif

    if(!holdState) {
      if(armState) {
        clockTicks = 1;
#include "arm-new.h"
      } else {
#include "thumb.h"
      }
    } else {
      clockTicks = lcdTicks;

      if(soundTicks < clockTicks)
        clockTicks = soundTicks;
      
      if(timer0On && (timer0Ticks < clockTicks)) {
        clockTicks = timer0Ticks;
      }
      if(timer1On && (timer1Ticks < clockTicks)) {
        clockTicks = timer1Ticks;
      }
      if(timer2On && (timer2Ticks < clockTicks)) {
        clockTicks = timer2Ticks;
      }
      if(timer3On && (timer3Ticks < clockTicks)) {
        clockTicks = timer3Ticks;
      }
#ifdef PROFILING
      if(profilingTicksReload != 0) {
        if(profilingTicks < clockTicks) {
          clockTicks = profilingTicks;
        }
      }
#endif
    }

    clockTicks += cpuMemoryWait[(reg[15].I >> 24) & 15];
    
    cpuLoopTicks -= clockTicks;
    if((cpuLoopTicks <= 0)) {
      if(cpuSavedTicks) {
        if(cpuLoopTicks <= 0)
          clockTicks = cpuSavedTicks;// + cpuLoopTicks;
        else
          clockTicks = cpuSavedTicks - cpuLoopTicks;
      }

    updateLoop:
      lcdTicks -= clockTicks;
      
      if(lcdTicks <= 0) {
        if(DISPSTAT & 1) { // V-BLANK
          // if in V-Blank mode, keep computing...
          if(DISPSTAT & 2) {
            lcdTicks += 960;
            VCOUNT++;
            UPDATE_REG(0x06, VCOUNT);
            DISPSTAT &= 0xFFFD;
            UPDATE_REG(0x04, DISPSTAT);
            CPUCompareVCOUNT();
          } else {
            lcdTicks += 272;
            DISPSTAT |= 2;
            UPDATE_REG(0x04, DISPSTAT);
            if(DISPSTAT & 16) {
              IF |= 2;
              UPDATE_REG(0x202, IF);
            }
          }
          
          if(VCOUNT >= 228) {
            DISPSTAT &= 0xFFFC;
            UPDATE_REG(0x04, DISPSTAT);
            VCOUNT = 0;
            UPDATE_REG(0x06, VCOUNT);
            CPUCompareVCOUNT();
          }
        } else {
          int framesToSkip = systemFrameSkip;
          if(speedup)
            framesToSkip = 9; // try 6 FPS during speedup
          
          if(DISPSTAT & 2) {
            // if in H-Blank, leave it and move to drawing mode
            VCOUNT++;
            UPDATE_REG(0x06, VCOUNT);
            
            lcdTicks += (960);
            DISPSTAT &= 0xFFFD;
            if(VCOUNT == 160) {
              count++;
              if((count % 10) == 0) {
                system10Frames(60);
              }
              if(count == 60) {
                u32 time = systemGetClock();
                if(time != lastTime) {
                  u32 t = 100000/(time - lastTime);
                  systemShowSpeed(t);
                } else
                  systemShowSpeed(0);
                lastTime = time;
                count = 0;
              }
              u32 joy = 0;
              // update joystick information
              if(systemReadJoypads())
                // read default joystick
                joy = systemReadJoypad(-1);
              P1 = 0x03FF ^ (joy & 0x3FF);
              if(cpuEEPROMSensorEnabled)
                systemUpdateMotionSensor();              
              UPDATE_REG(0x130, P1);
              u16 P1CNT = FROM16LE(*((u16 *)&ioMem[0x132]));
              if(P1CNT & 0x4000) {
                u16 p1 = (0x3FF ^ P1) & 0x3FF;
                if(P1CNT & 0x8000) {
                  if(p1 == (P1CNT & 0x3FF)) {
                    IF |= 0x1000;
                    UPDATE_REG(0x202, IF);
                  }
                } else {
                  if(p1 & P1CNT) {
                    IF |= 0x1000;
                    UPDATE_REG(0x202, IF);
                  }
                }
              }
              
              u32 ext = (joy >> 10);
              cheatsCheckKeys(P1^0x3FF, ext);
              speedup = (ext & 1) ? true : false;
              capture = (ext & 2) ? true : false;
              
              if(capture && !capturePrevious) {
                captureNumber++;
                systemScreenCapture(captureNumber);
              }
              capturePrevious = capture;
              
              DISPSTAT |= 1;
              DISPSTAT &= 0xFFFD;
              UPDATE_REG(0x04, DISPSTAT);
              if(DISPSTAT & 0x0008) {
                IF |= 1;
                UPDATE_REG(0x202, IF);
              }
              CPUCheckDMA(1, 0x0f);
              if(frameCount >= framesToSkip) {
                systemDrawScreen();
                frameCount = 0;
              } else 
                frameCount++;
              if(systemPauseOnFrame())
                ticks = 0;
            }
            
            UPDATE_REG(0x04, DISPSTAT);
            
            CPUCompareVCOUNT(); 
          } else {
            if(frameCount >= framesToSkip) {
              (*renderLine)();
              
              switch(systemColorDepth) {
              case 16:
                {
                  u16 *dest = (u16 *)pix + 241 * (VCOUNT+1);
                  for(int x = 0; x < 240;) {
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                    *dest++ = systemColorMap16[lineMix[x++]&0xFFFF];
                  }
                  // for filters that read past the screen
                  *dest++ = 0;
                }
                break;
              case 24:
                {
                  u8 *dest = (u8 *)pix + 240 * VCOUNT * 3;
                  for(int x = 0; x < 240;) {
                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;
                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;
                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;
                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;

                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;
                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;
                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;
                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;

                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;
                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;
                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;
                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;

                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;
                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;
                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;
                    *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                    dest += 3;              
                  }
                }
                break;
              case 32:
                {
                  u32 *dest = (u32 *)pix + 241 * (VCOUNT+1);
                  for(int x = 0; x < 240; ) {
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                    *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];
                  }
                }
                break;
              }
            }
            // entering H-Blank
            DISPSTAT |= 2;
            UPDATE_REG(0x04, DISPSTAT);
            lcdTicks += 272;
            CPUCheckDMA(2, 0x0f);
            if(DISPSTAT & 16) {
              IF |= 2;
              UPDATE_REG(0x202, IF);
            }
          }
        }       
      }

      if(!stopState) {
        if(timer0On) {
          if(timer0ClockReload == 1) {
            u32 tm0d = TM0D + clockTicks;
            if(tm0d > 0xffff) {
              tm0d += timer0Reload;
              timerOverflow |= 1;
              soundTimerOverflow(0);
              if(TM0CNT & 0x40) {
                IF |= 0x08;
                UPDATE_REG(0x202, IF);
              }
            }
            TM0D = tm0d;
            timer0Ticks = 0x10000 - TM0D;
            UPDATE_REG(0x100, TM0D);            
          } else {
            timer0Ticks -= clockTicks;    
            if(timer0Ticks <= 0) {
              timer0Ticks += timer0ClockReload;
              TM0D++;
              if(TM0D == 0) {
                TM0D = timer0Reload;
                timerOverflow |= 1;
                soundTimerOverflow(0);
                if(TM0CNT & 0x40) {
                  IF |= 0x08;
                  UPDATE_REG(0x202, IF);
                }
              }
              UPDATE_REG(0x100, TM0D);  
            }
          }
        }
        
        if(timer1On) {
          if(TM1CNT & 4) {
            if(timerOverflow & 1) {
              TM1D++;
              if(TM1D == 0) {
                TM1D += timer1Reload;
                timerOverflow |= 2;
                soundTimerOverflow(1);
                if(TM1CNT & 0x40) {
                  IF |= 0x10;
                  UPDATE_REG(0x202, IF);
                }
              }
              UPDATE_REG(0x104, TM1D);
            }
          } else {
            if(timer1ClockReload == 1) {
              u32 tm1d = TM1D + clockTicks;
              if(tm1d > 0xffff) {
                tm1d += timer1Reload;
                timerOverflow |= 2;           
                soundTimerOverflow(1);
                if(TM1CNT & 0x40) {
                  IF |= 0x10;
                  UPDATE_REG(0x202, IF);
                }
              }
              TM1D = tm1d;
              timer1Ticks = 0x10000 - TM1D;
              UPDATE_REG(0x104, TM1D);                    
            } else {
              timer1Ticks -= clockTicks;          
              if(timer1Ticks <= 0) {
                timer1Ticks += timer1ClockReload;
                TM1D++;
                
                if(TM1D == 0) {
                  TM1D = timer1Reload;
                  timerOverflow |= 2;           
                  soundTimerOverflow(1);
                  if(TM1CNT & 0x40) {
                    IF |= 0x10;
                    UPDATE_REG(0x202, IF);
                  }
                }
                UPDATE_REG(0x104, TM1D);        
              }
            }
          }
        }
        
        if(timer2On) {
          if(TM2CNT & 4) {
            if(timerOverflow & 2) {
              TM2D++;
              if(TM2D == 0) {
                TM2D += timer2Reload;
                timerOverflow |= 4;
                if(TM2CNT & 0x40) {
                  IF |= 0x20;
                  UPDATE_REG(0x202, IF);
                }
              }
              UPDATE_REG(0x108, TM2D);
            }
          } else {
            if(timer2ClockReload == 1) {
              u32 tm2d = TM2D + clockTicks;
              if(tm2d > 0xffff) {
                tm2d += timer2Reload;
                timerOverflow |= 4;
                if(TM2CNT & 0x40) {
                  IF |= 0x20;
                  UPDATE_REG(0x202, IF);
                }
              }
              TM2D = tm2d;
              timer2Ticks = 0x10000 - TM2D;
              UPDATE_REG(0x108, TM2D);                    
            } else {
              timer2Ticks -= clockTicks;          
              if(timer2Ticks <= 0) {
                timer2Ticks += timer2ClockReload;
                TM2D++;
                
                if(TM2D == 0) {
                  TM2D = timer2Reload;
                  timerOverflow |= 4;
                  if(TM2CNT & 0x40) {
                    IF |= 0x20;
                    UPDATE_REG(0x202, IF);
                  }
                }
                UPDATE_REG(0x108, TM2D);        
              }
            }
          }
        }
        
        if(timer3On) {
          if(TM3CNT & 4) {
            if(timerOverflow & 4) {
              TM3D++;
              if(TM3D == 0) {
                TM3D += timer3Reload;
                if(TM3CNT & 0x40) {
                  IF |= 0x40;
                  UPDATE_REG(0x202, IF);
                }
              }
              UPDATE_REG(0x10c, TM3D);
            }
          } else {
            if(timer3ClockReload == 1) {
              u32 tm3d = TM3D + clockTicks;
              if(tm3d > 0xffff) {
                tm3d += timer3Reload;
                if(TM3CNT & 0x40) {
                  IF |= 0x40;
                  UPDATE_REG(0x202, IF);
                }
              }
              TM3D = tm3d;
              timer3Ticks = 0x10000 - TM3D;
              UPDATE_REG(0x10C, TM3D);                            
            } else {
              timer3Ticks -= clockTicks;          
              if(timer3Ticks <= 0) {
                timer3Ticks += timer3ClockReload;
                TM3D++;
                
                if(TM3D == 0) {
                  TM3D = timer3Reload;
                  if(TM3CNT & 0x40) {
                    IF |= 0x40;
                    UPDATE_REG(0x202, IF);
                  }
                }
                UPDATE_REG(0x10C, TM3D);
              }
            }
          }
        }
        
        soundTicks -= clockTicks;
        if(soundTicks <= 0) {
          soundTick();
          soundTicks += SOUND_CLOCK_TICKS;
        }
        timerOverflow = 0;
      }

#ifdef PROFILING
      profilingTicks -= clockTicks;
      if(profilingTicks <= 0) {
        profilingTicks += profilingTicksReload;
        if(profilBuffer && profilSize) {
          u16 *b = (u16 *)profilBuffer;
          int pc = ((reg[15].I - profilLowPC) * profilScale)/0x10000;
          if(pc >= 0 && pc < profilSize) {
            b[pc]++;
          }
        }
      }
#endif

      ticks -= clockTicks;

      CPUUpdateTicks(cpuLoopTicks);
      
      if(cpuDmaTicksToUpdate > 0) {
        clockTicks = cpuSavedTicks;
        if(clockTicks > cpuDmaTicksToUpdate)
          clockTicks = cpuDmaTicksToUpdate;
        cpuDmaTicksToUpdate -= clockTicks;
        if(cpuDmaTicksToUpdate < 0)
          cpuDmaTicksToUpdate = 0;
        goto updateLoop;
      }

      if(IF && (IME & 1) && armIrqEnable) {
        int res = IF & IE;
        if(stopState)
          res &= 0x3080;
        if(res) {
          if(intState) {
            CPUInterrupt();         
            intState = false;
            if(holdState) {
              holdState = false;
              stopState = false;
            }       
          } else {
            if(!holdState) {
              intState = true;
              cpuLoopTicks = 5;
              cpuSavedTicks = 5;
            } else {
              CPUInterrupt();         
              if(holdState) {
                holdState = false;
                stopState = false;
              }
            }
          }
        }
      }
      
      if(ticks <= 0)
        break;
    }
  }
}
