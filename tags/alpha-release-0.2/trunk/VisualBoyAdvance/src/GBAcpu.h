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

#ifndef VBA_GBAcpu_H
#define VBA_GBAcpu_H

extern int armExecute();
extern int thumbExecute();

#ifdef __GNUC__
# define LIKELY(x) __builtin_expect(!!(x),1)
# define UNLIKELY(x) __builtin_expect(!!(x),0)
#else
# define LIKELY(x) (x)
# define UNLIKELY(x) (x)
#endif

#if defined(__GNUC__) && !defined(__POWERPC__)
# define INSN_REGPARM __attribute__((regparm(1)))
#else
# define INSN_REGPARM /*nothing*/
#endif


#define UPDATE_REG(address, value)\
  {\
    WRITE16LE(((u16 *)&ioMem[address]),value);\
  }\

#define ARM_PREFETCH \
  {\
    cpuPrefetch[0] = CPUReadMemoryQuick(armNextPC);\
    cpuPrefetch[1] = CPUReadMemoryQuick(armNextPC+4);\
  }

#define THUMB_PREFETCH \
  {\
    cpuPrefetch[0] = CPUReadHalfWordQuick(armNextPC);\
    cpuPrefetch[1] = CPUReadHalfWordQuick(armNextPC+2);\
  }

#define ARM_PREFETCH_NEXT \
  cpuPrefetch[1] = CPUReadMemoryQuick(armNextPC+4);

#define THUMB_PREFETCH_NEXT\
  cpuPrefetch[1] = CPUReadHalfWordQuick(armNextPC+2);


extern int SWITicks;
extern u32 mastercode;
extern bool busPrefetch;
extern bool busPrefetchEnable;
extern u32 busPrefetchCount;
extern int cpuNextEvent;
extern bool holdState;
extern u32 cpuPrefetch[2];
extern int cpuTotalTicks;
extern u8 memoryWait[16];
extern u8 memoryWait32[16];
extern u8 memoryWaitSeq[16];
extern u8 memoryWaitSeq32[16];
extern u8 cpuBitsSet[256];
extern u8 cpuLowestBitSet[256];
extern void CPUSwitchMode(int mode, bool saveState, bool breakLoop);
extern void CPUSwitchMode(int mode, bool saveState);
extern void CPUUpdateCPSR();
extern void CPUUpdateFlags(bool breakLoop);
extern void CPUUpdateFlags();
extern void CPUUndefinedException();
extern void CPUSoftwareInterrupt();
extern void CPUSoftwareInterrupt(int comment);


// Waitstates when accessing data
inline int dataTicksAccess16(u32 address) // DATA 8/16bits NON SEQ
{
  int addr = (address>>24)&15;
  int value =  memoryWait[addr];

  if (addr>=0x08)
  {
    busPrefetchCount=0;
    busPrefetch=false;
  }
  else if (busPrefetch)
  {
    int waitState = value;
    if (waitState>0)
      waitState--;
    waitState++;
    busPrefetchCount = (busPrefetchCount<<waitState) | (0xFF>>(8-waitState));
  }

  return value;
}

inline int dataTicksAccess32(u32 address) // DATA 32bits NON SEQ
{
  int addr = (address>>24)&15;
  int value = memoryWait32[addr];

  if (addr>=0x08)
  {
    busPrefetchCount=0;
    busPrefetch=false;
  }
  else if (busPrefetch)
  {
    int waitState = value;
    if (waitState>0)
      waitState--;
    waitState++;
    busPrefetchCount = (busPrefetchCount<<waitState) | (0xFF>>(8-waitState));
  }

  return value;
}

inline int dataTicksAccessSeq16(u32 address)// DATA 8/16bits SEQ
{
  int addr = (address>>24)&15;
  int value = memoryWaitSeq[addr];

  if (addr>=0x08)
  {
    busPrefetchCount=0;
    busPrefetch=false;
  }
  else if (busPrefetch)
  {
    int waitState = value;
    if (waitState>0)
      waitState--;
    waitState++;
    busPrefetchCount = (busPrefetchCount<<waitState) | (0xFF>>(8-waitState));
  }

  return value;
}

inline int dataTicksAccessSeq32(u32 address)// DATA 32bits SEQ
{
  int addr = (address>>24)&15;
  int value =  memoryWaitSeq32[addr];

  if (addr>=0x08)
  {
    busPrefetchCount=0;
    busPrefetch=false;
  }
  else if (busPrefetch)
  {
    int waitState = value;
    if (waitState>0)
      waitState--;
    waitState++;
    busPrefetchCount = (busPrefetchCount<<waitState) | (0xFF>>(8-waitState));
  }

  return value;
}

inline int dataTicksAccessMulti32(u32 address, int count)// DATA 32bits NONSEQ+SEQ
{
  int addr = (address>>24)&15;
  int value1 = memoryWait32[addr];
  int value2 = memoryWaitSeq32[addr];
  int ret = value1 + value2*(count-1);

  if (addr>=0x08)
  {
    busPrefetchCount=0;
    busPrefetch=false;
  }
  else if (busPrefetch)
  {
    if (!value1)
      value1++;
    if (!value2)
      value2++;
    int waitState = value1 + value2*(count-1);
    if (waitState < 32)
      busPrefetchCount = (busPrefetchCount<<waitState) | (~0>>(32-waitState));
    else
      busPrefetchCount = ~0;
  }

  return ret;
}


// Waitstates when executing opcode
inline int codeTicksAccess16(u32 address) // THUMB NON SEQ
{
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D) && (busPrefetchCount&1))
  {
    busPrefetchCount=((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
    return busPrefetchCount&1 ? 0 : memoryWaitSeq[addr]-1;
  }
  else
  {
    busPrefetchCount = 0;
    return memoryWait[addr];
  }
}

inline int codeTicksAccess32(u32 address) // ARM NON SEQ
{
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D) && (busPrefetchCount&1))
  {
    busPrefetchCount=((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
    if (busPrefetchCount&0x1)
    {
      busPrefetchCount=((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
      return 0;
    }
    else
    {
      busPrefetchCount = 0;
      return memoryWaitSeq[addr];
    }
  }
  else
  {
    busPrefetchCount = 0;
    return memoryWait32[addr];
  }
}

inline int codeTicksAccessSeq16(u32 address) // THUMB SEQ
{
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if (busPrefetchCount&0x1)
    {
      busPrefetchCount=((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
      return 0;
    }
    else
    if (busPrefetchCount&0xFFFFFF00)
    {
      busPrefetchCount=0;
      return memoryWait[addr];
    }
    else
      return memoryWaitSeq[addr];
  }
  else
  {
    busPrefetchCount = 0;
    return memoryWaitSeq[addr];
  }
}

inline int codeTicksAccessSeq32(u32 address) // ARM SEQ
{
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if (busPrefetchCount&0x1)
    {
      busPrefetchCount=((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
      if (busPrefetchCount&0x1)
      {
        busPrefetchCount=((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
        return 0;
      }
      else
        return memoryWaitSeq[addr];

    }
    else
    if (busPrefetchCount>0xFF)
    {
      busPrefetchCount=0;
      return memoryWait32[addr];
    }
    else
      return memoryWaitSeq32[addr];
  }
  else
  {
    return memoryWaitSeq32[addr];
  }
}


// Emulates the Cheat System (m) code
inline void cpuMasterCodeCheck()
{
  if((cheatsEnabled) && (mastercode!=0) && (mastercode == armNextPC))
  {
    u32 joy = 0;
    if(systemReadJoypads())
      joy = systemReadJoypad(-1);
    u32 ext = (joy >> 10);
    cpuTotalTicks += cheatsCheckKeys(P1^0x3FF, ext);
  }
}

#endif //VBA_GBAcpu_H
