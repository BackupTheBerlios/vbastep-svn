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

extern char cpuBreakLoop;
extern char holdState;
extern u32 cpuPrefetch[2];
extern u32 mastercode;
extern char busPrefetch;
extern int busPrefetchCount;
extern char busPrefetchEnable;
extern char timer0On, timer1On, timer2On, timer3On;
extern int timer0Ticks, timer1Ticks, timer2Ticks, timer3Ticks;
extern int timer0Reload, timer1Reload, timer2Reload, timer3Reload;
extern int timer0ClockReload, timer1ClockReload, timer2ClockReload, timer3ClockReload;
extern int profilingTicks, profilingTicksReload;
extern char instaEvent;
extern int lcdTicks;
extern u8 cpuBitsSet[256];
extern int cpuNextEvent;
extern int count;
extern u32 lastTime;
extern char stopState;
extern int capture;
extern int capturePrevious;
extern int captureNumber;
extern int frameCount;
extern void(*renderLine)();
extern int cpuDmaTicksToUpdate;

extern u8 memoryWait[16];
extern u8 memoryWait32[16];
extern u8 memoryWaitSeq[16];
extern u8 memoryWaitSeq32[16];

extern void CPUSwitchMode(int, char, char);
extern void CPUSwitchMode(int, char);
extern void CPUUpdateCPSR();
extern void CPUUpdateFlags(char);
extern void CPUUpdateFlags();
extern void CPUSoftwareInterrupt(int);
extern void CPUSoftwareInterrupt();
extern void CPUUndefinedException();
extern void CPUCompareVCOUNT();
extern void CPUInterrupt();

#define UPDATE_REG(address, value) WRITE16LE(((u16 *)&ioMem[address]),value)

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

// Waitstates when executing opcode
inline int codeTicksAccess16(u32 address) // THUMB NON SEQ
{
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if ((!(busPrefetch)) && (busPrefetchEnable) && (busPrefetchCount))
    {
        busPrefetchCount-=1;
      return 0;
    }
    else if (busPrefetch)
    {
      busPrefetchCount-=2;
      return 0;
    }
    else
      return memoryWait[addr];
  }
  else if (addr!=2)
  {
    busPrefetchCount = 0;
    return memoryWait[addr];
  }
  else
  {
    busPrefetchCount = 0;
    return memoryWait[addr] + 2;
  }
}

inline int codeTicksAccess32(u32 address) // ARM NON SEQ
{
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if ((!(busPrefetch)) && (busPrefetchEnable) && (busPrefetchCount))
    {
      busPrefetchCount-=1;
      return 0;
    }
    else if (busPrefetch)
    {
      busPrefetchCount-=2;
      return 0;
    }
    else
      return memoryWait32[addr];
  }
  else if (addr!=2)
  {
    busPrefetchCount = 0;
    return memoryWait32[addr];
  }
  else
  {
    busPrefetchCount = 0;
    return memoryWait32[addr] + 2;
  }
}

inline int codeTicksAccessSeq16(u32 address) // THUMB SEQ
{
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if ((!(busPrefetch)) && (busPrefetchEnable) && (busPrefetchCount))
    {
      busPrefetchCount-=1;
      return 0;
    }
    else if (busPrefetch)
    {
      busPrefetchCount-=2;
      return 0;
    }
    else
      return memoryWaitSeq[addr];
  }
  else if (addr!=2)
  {
    busPrefetchCount = 0;
    return memoryWaitSeq[addr];
  }
  else
  {
    busPrefetchCount = 0;
    return memoryWaitSeq[addr] + 2;
  }
}

inline int codeTicksAccessSeq32(u32 address) // ARM SEQ
{
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if ((!(busPrefetch)) && (busPrefetchEnable) && (busPrefetchCount))
    {
      busPrefetchCount-=1;
      return 0;
    }
    else if (busPrefetch)
    {
      busPrefetchCount-=2;
      return 0;
    }
    else
      return memoryWaitSeq32[addr];
  }
  else if (addr!=2)
  {
    return memoryWaitSeq32[addr];
  }
  else
  {
    return memoryWaitSeq32[addr] + 4;
  }
}


inline int CPUUpdateTicks()
{
  int cpuLoopTicks = lcdTicks;
  
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

  //Used for DMA, or when an IRQ has been enabled
  if (instaEvent)
  {
    cpuLoopTicks = 0;
    instaEvent = false;
  }

  return cpuLoopTicks;
}


// Waitstates when reading data
inline int dataTicksAccess16(u32 address) // DATA 8/16bits NON SEQ
{
  int addr = (address>>24)&15;
  int value =  memoryWait[addr];
  if ((addr>=0x08) && (addr<=0x0E))
  {
    busPrefetchCount=0;
    busPrefetch=false;
  }
  else
  {
  //  if ((addr>=0x05) && (addr<=0x07))
  //  {
  //    char blank = (((DISPSTAT | (DISPSTAT>>1))&1==1) ?  true : false);
  //    char pixelDrawing = (((lcdTicks+40-clockTicks) & 3) == 0 ? true : false);
  //    value += ((!blank && pixelDrawing) ? videoMemoryWait[addr] : 0);
  //  }
    busPrefetchCount+= value+1;
  }

  return value;
}

inline int dataTicksAccess32(u32 address) // DATA 32bits NON SEQ
{
  int addr = (address>>24)&15;
  int value = memoryWait32[addr];

  if ((addr>=0x08) && (addr<=0x0E))
  {
    busPrefetchCount=0;
    busPrefetch=false;
  }
  else
  {
  //  if ((addr>=0x05) && (addr<=0x07))
  //  {
  //    char blank = (((DISPSTAT | (DISPSTAT>>1))&1==1) ?  true : false);
  //    char pixelDrawing = (((lcdTicks+40-clockTicks) & 3) == 0 ? true : false);
  //    value += ((!blank && pixelDrawing) ? videoMemoryWait[addr] : 0);
  //  }
    busPrefetchCount+= value+1;
  }

  return value;
}

inline int dataTicksAccessSeq16(u32 address)// DATA 8/16bits SEQ
{
  int addr = (address>>24)&15;
  int value = memoryWaitSeq[addr]+1;

  if ((addr>=0x08) && (addr<=0x0E))
  {
    busPrefetchCount=0;
    busPrefetch=false;
  }
  else
  {
    if ((addr>=0x05) && (addr<=0x07))
  //  {
  //    char blank = (((DISPSTAT | (DISPSTAT>>1))&1==1) ?  true : false);
  //    char pixelDrawing = (((lcdTicks+40-clockTicks) & 3) == 0 ? true : false);
  //    value += ((!blank && pixelDrawing) ? videoMemoryWait[addr] : 0);
  //  }
    busPrefetchCount+= value+1;
  }

  return value;
}

inline int dataTicksAccessSeq32(u32 address)// DATA 32bits SEQ
{
  int addr = (address>>24)&15;
  int value =  memoryWaitSeq32[addr]+1;

  if ((addr>=0x08) && (addr<=0x0E))
  {
    busPrefetchCount=0;
    busPrefetch=false;
  }
  else
  {
  //  if ((addr>=0x05) && (addr<=0x07))
  //  {
  //    char blank = (((DISPSTAT | (DISPSTAT>>1))&1==1) ?  true : false);
  //    char pixelDrawing = (((lcdTicks+40-clockTicks) & 3) == 0 ? true : false);
  //    value += ((!blank && pixelDrawing) ? videoMemoryWait[addr] : 0);
  //  }
    busPrefetchCount+= value+1;
  }

  return value;
}

