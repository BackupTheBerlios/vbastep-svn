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
#include "Util.h"
#include "Port.h"
#include "agbprint.h"
#ifdef PROFILING
#include "prof/prof.h"
#endif

#include "gbaint.h"

#ifdef PROFILING
int profilingTicks = 0;
int profilingTicksReload = 0;
static char *profilBuffer = NULL;
static int profilSize = 0;
static u32 profilLowPC = 0;
static int profilScale = 0;


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

void CPULoop(int ticks)
{  
  int clockTicks;
  int totalTicks = 0;
  int timerOverflow = 0;
  // variable used by the CPU core
  cpuBreakLoop = false;

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
        emulog(buffer);
      } else if(!holdState) {
        sprintf(buffer, "PC=%08x\n", armNextPC);
        emulog(buffer);
      }
    }
#endif

    if(!holdState) {

      // Emulates the Cheat System (m) code
      if((cheatsEnabled) && (mastercode!=0) && (mastercode == armNextPC))
      {
        u32 joy = 0;
        if(systemReadJoypads())
          joy = systemReadJoypad(-1);
        u32 ext = (joy >> 10);
        totalTicks += cheatsCheckKeys(P1^0x3FF, ext);
      }

      if(armState) {
#include "arm-new.h"
      } else {
#include "thumb.h"
      }
    } else
      clockTicks = CPUUpdateTicks();

    totalTicks += clockTicks;

    cpuNextEvent = CPUUpdateTicks();

    if(cpuNextEvent > ticks)
      cpuNextEvent = ticks;

    if(totalTicks >= cpuNextEvent) {
      int remainingTicks = totalTicks - cpuNextEvent;
      clockTicks = cpuNextEvent;
      totalTicks = 0;
      cpuDmaHack = false;

    
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
          
          if(VCOUNT >= 228) { //Reaching last line
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
              systemFrame();
              
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
              u16 P1CNT = READ16LE(((u16 *)&ioMem[0x132]));
              // this seems wrong, but there are cases where the game
              // can enter the stop state without requesting an IRQ from
              // the joypad.
              if((P1CNT & 0x4000) || stopState) {
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
              // If no (m) code is enabled, apply the cheats at each LCDline
              if((cheatsEnabled) && (mastercode==0))
                remainingTicks += cheatsCheckKeys(P1^0x3FF, ext);
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
            if(frameCount >= framesToSkip) 
            {
              (*renderLine)();
              
              switch(systemColorDepth) {
              case 16:
                {
                  u16 *dest = (u16 *)pix + 242 * (VCOUNT+1);
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
            if(TM0D < timer0Reload)
              TM0D = timer0Reload;
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
              if(TM1D < timer1Reload)
                TM1D = timer1Reload;
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
              if(TM2D < timer2Reload)
                TM2D = timer2Reload;
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
              if(TM3D < timer3Reload)
                TM3D = timer3Reload;
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
      }
      timerOverflow = 0;

      // we shouldn't be doing sound in stop state, but we lose synchronization
      // if sound is disabled, so in stop state, soundTick will just produce
      // mute sound
      soundTicks -= clockTicks;
      if(soundTicks <= 0) {
        soundTick();
        soundTicks += SOUND_CLOCK_TICKS;
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

      cpuNextEvent = CPUUpdateTicks();
      
      if(cpuDmaTicksToUpdate > 0) {
        if(cpuDmaTicksToUpdate > cpuNextEvent)
          clockTicks = cpuNextEvent;
        else
          clockTicks = cpuDmaTicksToUpdate;
        cpuDmaTicksToUpdate -= clockTicks;
        if(cpuDmaTicksToUpdate < 0)
          cpuDmaTicksToUpdate = 0;
        cpuDmaHack = true;
        goto updateLoop;
      }

      if(IF && (IME & 1) && armIrqEnable) {
        int res = IF & IE;
        if(stopState)
          res &= 0x3080;
        if(res) {
          remainingTicks+=5;
          CPUInterrupt();
          holdState = false;
          stopState = false;
        }
      }

      if(remainingTicks > 0) {
        if(remainingTicks > cpuNextEvent)
          clockTicks = cpuNextEvent;
        else
          clockTicks = remainingTicks;
        remainingTicks -= clockTicks;
        if(remainingTicks < 0)
          remainingTicks = 0;
        goto updateLoop;
      }
      
      if(ticks <= 0 || cpuBreakLoop)
        break;
    }
    else
    // Update in-game timer value after each instruction
    // (in case a game reads them). Still not working good ?
    if(totalTicks < cpuNextEvent) {
      if(timer0On && (timer0Ticks < clockTicks)) {
       UPDATE_REG(0x100, TM0D+totalTicks);
      }
      if(timer1On && !(TM1CNT & 4) && (timer1Ticks < clockTicks)) {
        UPDATE_REG(0x104, TM1D+totalTicks);
      }
      if(timer2On && !(TM2CNT & 4) && (timer2Ticks < clockTicks)) {
        UPDATE_REG(0x108, TM2D+totalTicks);
      }
      if(timer3On && !(TM3CNT & 4) && (timer3Ticks < clockTicks)) {
        UPDATE_REG(0x10C, TM3D+totalTicks);
      }
    } 
  }
}
