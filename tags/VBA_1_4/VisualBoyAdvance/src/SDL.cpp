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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "AutoBuild.h"

#include "SDL.h"
#include "GBA.h"
#include "Flash.h"
#include "Port.h"
#include "Font.h"
#include "debugger.h"
#include "Sound.h"
#include "unzip.h"
#include "Util.h"
#include "gb/GB.h"
#include "gb/gbGlobals.h"
#ifdef GP_EMULATION
#include "gp/GP.h"
#endif

#ifdef __GNUC__
#include <unistd.h>
#define GETCWD getcwd
#else
#include <direct.h>
#define GETCWD _getcwd
#endif

#ifndef __GNUC__
#define HAVE_DECL_GETOPT 0
#define __STDC__ 1
#include "getopt.h"
#endif

#ifdef MMX
extern "C" bool cpu_mmx;
#endif
extern bool soundEcho;
extern bool soundLowPass;
extern bool soundReverse;
extern int Init_2xSaI(u32);
extern void _2xSaI(u8*,u32,u8*,u8*,u32,int,int);
extern void _2xSaI32(u8*,u32,u8*,u8*,u32,int,int);  
extern void Super2xSaI(u8*,u32,u8*,u8*,u32,int,int);
extern void Super2xSaI32(u8*,u32,u8*,u8*,u32,int,int);
extern void SuperEagle(u8*,u32,u8*,u8*,u32,int,int);
extern void SuperEagle32(u8*,u32,u8*,u8*,u32,int,int);  
extern void TVMode(u8*,u32,u8*,u8*,u32,int,int);
extern void TVMode32(u8*,u32,u8*,u8*,u32,int,int);
extern void Pixelate(u8*,u32,u8*,u8*,u32,int,int);
extern void Pixelate32(u8*,u32,u8*,u8*,u32,int,int);
extern void MotionBlur(u8*,u32,u8*,u8*,u32,int,int);
extern void MotionBlur32(u8*,u32,u8*,u8*,u32,int,int);
extern void AdMame2x(u8*,u32,u8*,u8*,u32,int,int);
extern void AdMame2x32(u8*,u32,u8*,u8*,u32,int,int);
extern void Simple2x(u8*,u32,u8*,u8*,u32,int,int);
extern void Simple2x32(u8*,u32,u8*,u8*,u32,int,int);
extern void Bilinear(u8*,u32,u8*,u8*,u32,int,int);
extern void Bilinear32(u8*,u32,u8*,u8*,u32,int,int);
extern void BilinearPlus(u8*,u32,u8*,u8*,u32,int,int);
extern void BilinearPlus32(u8*,u32,u8*,u8*,u32,int,int);

extern void SmartIB(u8*,u32,int,int);
extern void SmartIB32(u8*,u32,int,int);
extern void MotionBlurIB(u8*,u32,int,int);
extern void MotionBlurIB32(u8*,u32,int,int);

void Init_Overlay(SDL_Surface *surface, int overlaytype);
void Quit_Overlay(void);
void Draw_Overlay(SDL_Surface *surface, int size);

extern void remoteInit();
extern void remoteCleanUp();
extern void remoteStubMain();
extern void remoteStubSignal(int,int);
extern void remoteOutput(char *, u32);
extern void remoteSetProtocol(int);
extern void remoteSetPort(int);
extern void debuggerOutput(char *, u32);

extern void CPUUpdateCPSR();
#ifdef GP_EMULATION
extern void GPUpdateCPSR();
#endif

bool (*emuWriteState)(char *) = NULL;
bool (*emuReadState)(char *) = NULL;
bool (*emuWriteBattery)(char *) = NULL;
bool (*emuReadBattery)(char *) = NULL;
void (*emuReset)() = NULL;
void (*emuCleanUp)() = NULL;
bool (*emuWritePNG)(char *) = NULL;
bool (*emuWriteBMP)(char *) = NULL;
void (*emuMain)(int) = NULL;
void (*emuUpdateCPSR)() = NULL;
int emuCount = 0;
bool emuHasDebugger = false;

static u8 COPYRIGHT[] = {
  0xa9, 0x96, 0x8c, 0x8a, 0x9e, 0x93, 0xbd, 0x90, 0x86, 0xbe, 0x9b, 0x89,
  0x9e, 0x91, 0x9c, 0x9a, 0xdf, 0xd7, 0xbc, 0xd6, 0xdf, 0xce, 0xc6, 0xc6,
  0xc6, 0xd3, 0xcd, 0xcf, 0xcf, 0xcf, 0xd3, 0xcd, 0xcf, 0xcf, 0xce, 0xdf,
  0x9d, 0x86, 0xdf, 0xb9, 0x90, 0x8d, 0x98, 0x90, 0x8b, 0x8b, 0x9a, 0x91,
  0x00
};

SDL_Surface *surface = NULL;
SDL_Overlay *overlay = NULL;
SDL_Rect overlay_rect;

int systemSpeed = 0;
int systemRedShift = 0;
int systemBlueShift = 0;
int systemGreenShift = 0;
int systemColorDepth = 0;
int systemDebug = 0;
int systemVerbose = 0;
int systemFrameSkip = 0;

int srcPitch = 0;
int srcWidth = 0;
int srcHeight = 0;
int destWidth = 0;
int destHeight = 0;

int sensorX = 2047;
int sensorY = 2047;

int filter = 0;
u8 *delta = NULL;

int sdlPrintUsage = 0;
int disableMMX = 0;

int cartridgeType = 3;
int sizeOption = 0;
int captureFormat = 0;

int emulating = 0;
int RGB_LOW_BITS_MASK=0x821;
u32 systemColorMap32[0x10000];
u16 systemColorMap16[0x10000];
u16 systemGbPalette[24];
void (*filterFunction)(u8*,u32,u8*,u8*,u32,int,int) = NULL;
void (*ifbFunction)(u8*,u32,int,int) = NULL;
int ifbType = 0;
char filename[2048];
char ipsname[2048];
char biosFileName[2048];
char captureDir[2048];
char saveDir[2048];
char batteryDir[2048];

#define _stricmp strcasecmp

bool sdlButtons[4][12] = {
  { false, false, false, false, false, false, 
    false, false, false, false, false, false },
  { false, false, false, false, false, false,
    false, false, false, false, false, false },
  { false, false, false, false, false, false,
    false, false, false, false, false, false },
  { false, false, false, false, false, false,
    false, false, false, false, false, false }
};

bool sdlMotionButtons[4] = { false, false, false, false };

int sdlNumDevices = 0;
SDL_Joystick **sdlDevices = NULL;

bool wasPaused = false;
int autoFrameSkip = 0;
int frameskipadjust = 0;
int showRenderedFrames = 0;
int renderedFrames = 0;

int throttle = 0;
u32 throttleLastTime = 0;
u32 autoFrameSkipLastTime = 0;

int showSpeed = 1;
int showSpeedTransparent = 1;
bool disableStatusMessages = false;
bool paused = false;
bool pauseNextFrame = false;
bool debugger = false;
bool debuggerStub = false;
int fullscreen = 0;
bool systemSoundOn = false;
bool yuv = false;
int yuvType = 0;
bool removeIntros = false;
int sdlFlashSize = 0;
int sdlAutoIPS = 1;

int sdlDefaultJoypad = 0;

extern void debuggerSignal(int,int);

void (*dbgMain)() = debuggerMain;
void (*dbgSignal)(int,int) = debuggerSignal;
void (*dbgOutput)(char *, u32) = debuggerOutput;

int  mouseCounter = 0;
int autoFire = 0;
bool autoFireToggle = false;

bool screenMessage = false;
char screenMessageBuffer[21];
u32  screenMessageTime = 0;

SDL_cond *cond = NULL;
SDL_mutex *mutex = NULL;
u8 sdlBuffer[4096];
int sdlSoundLen = 0;

char *arg0;

#ifndef C_CORE
u8 sdlStretcher[16384];
int sdlStretcherPos;
#else
void (*sdlStretcher)(u8 *, u8*) = NULL;
#endif

enum {
  KEY_LEFT, KEY_RIGHT,
  KEY_UP, KEY_DOWN,
  KEY_BUTTON_A, KEY_BUTTON_B,
  KEY_BUTTON_START, KEY_BUTTON_SELECT,
  KEY_BUTTON_L, KEY_BUTTON_R,
  KEY_BUTTON_SPEED, KEY_BUTTON_CAPTURE
};

u16 joypad[4][12] = {
  { SDLK_LEFT,  SDLK_RIGHT,
    SDLK_UP,    SDLK_DOWN,
    SDLK_z,     SDLK_x,
    SDLK_RETURN,SDLK_BACKSPACE,
    SDLK_a,     SDLK_s,
    SDLK_SPACE, SDLK_F12
  },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

u16 defaultJoypad[12] = {
  SDLK_LEFT,  SDLK_RIGHT,
  SDLK_UP,    SDLK_DOWN,
  SDLK_z,     SDLK_x,
  SDLK_RETURN,SDLK_BACKSPACE,
  SDLK_a,     SDLK_s,
  SDLK_SPACE, SDLK_F12
};

u16 motion[4] = {
  SDLK_KP4, SDLK_KP6, SDLK_KP8, SDLK_KP2
};

u16 defaultMotion[4] = {
  SDLK_KP4, SDLK_KP6, SDLK_KP8, SDLK_KP2
};

struct option sdlOptions[] = {
  { "auto-frameskip", no_argument, &autoFrameSkip, 1 },  
  { "bios", required_argument, 0, 'b' },
  { "config", required_argument, 0, 'c' },
  { "debug", no_argument, 0, 'd' },
  { "filter", required_argument, 0, 'f' },
  { "filter-normal", no_argument, &filter, 0 },
  { "filter-tv-mode", no_argument, &filter, 1 },
  { "filter-2xsai", no_argument, &filter, 2 },
  { "filter-super-2xsai", no_argument, &filter, 3 },
  { "filter-super-eagle", no_argument, &filter, 4 },
  { "filter-pixelate", no_argument, &filter, 5 },
  { "filter-motion-blur", no_argument, &filter, 6 },
  { "filter-advmame", no_argument, &filter, 7 },
  { "filter-simple2x", no_argument, &filter, 8 },
  { "filter-bilinear", no_argument, &filter, 9 },
  { "filter-bilinear+", no_argument, &filter, 10 },  
  { "flash-size", required_argument, 0, 'S' },
  { "flash-64k", no_argument, &sdlFlashSize, 0 },
  { "flash-128k", no_argument, &sdlFlashSize, 1 },
  { "frameskip", required_argument, 0, 's' },
  { "fullscreen", no_argument, &fullscreen, 1 },
  { "gdb", required_argument, 0, 'G' },
  { "help", no_argument, &sdlPrintUsage, 1 },
  { "ifb-none", no_argument, &ifbType, 0 },
  { "ifb-motion-blur", no_argument, &ifbType, 1 },
  { "ifb-smart", no_argument, &ifbType, 2 },
  { "ips", required_argument, 0, 'i' },
  { "no-auto-frameskip", no_argument, &autoFrameSkip, 0 },
  { "no-debug", no_argument, 0, 'N' },
  { "no-ips", no_argument, &sdlAutoIPS, 0 },
  { "no-mmx", no_argument, &disableMMX, 1 },
  { "no-show-speed", no_argument, &showSpeed, 0 },
  { "no-throttle", no_argument, &throttle, 0 },
  { "profile", optional_argument, 0, 'p' },
  { "save-type", required_argument, 0, 't' },
  { "save-auto", no_argument, &cpuSaveType, 0 },
  { "save-eeprom", no_argument, &cpuSaveType, 1 },
  { "save-sram", no_argument, &cpuSaveType, 2 },
  { "save-flash", no_argument, &cpuSaveType, 3 },
  { "save-sensor", no_argument, &cpuSaveType, 4 },
  { "show-speed-normal", no_argument, &showSpeed, 1 },
  { "show-speed-detailed", no_argument, &showSpeed, 2 },
  { "throttle", required_argument, 0, 'T' },
  { "verbose", required_argument, 0, 'v' },  
  { "video-1x", no_argument, &sizeOption, 0 },
  { "video-2x", no_argument, &sizeOption, 1 },
  { "video-3x", no_argument, &sizeOption, 2 },
  { "video-4x", no_argument, &sizeOption, 3 },
  { "yuv", required_argument, 0, 'Y' },
  { NULL, no_argument, NULL, 0 }
};

extern bool CPUIsGBAImage(char *);
extern bool gbIsGameboyRom(char *);

#ifndef C_CORE
#define SDL_LONG(val) \
  *((u32 *)&sdlStretcher[sdlStretcherPos]) = val;\
  sdlStretcherPos+=4;

#define SDL_AND_EAX(val) \
  sdlStretcher[sdlStretcherPos++] = 0x25;\
  SDL_LONG(val);

#define SDL_AND_EBX(val) \
  sdlStretcher[sdlStretcherPos++] = 0x81;\
  sdlStretcher[sdlStretcherPos++] = 0xe3;\
  SDL_LONG(val);

#define SDL_OR_EAX_EBX \
  sdlStretcher[sdlStretcherPos++] = 0x09;\
  sdlStretcher[sdlStretcherPos++] = 0xd8;

#define SDL_LOADL_EBX \
  sdlStretcher[sdlStretcherPos++] = 0x8b;\
  sdlStretcher[sdlStretcherPos++] = 0x1f;

#define SDL_LOADW \
  sdlStretcher[sdlStretcherPos++] = 0x66;\
  sdlStretcher[sdlStretcherPos++] = 0x8b;\
  sdlStretcher[sdlStretcherPos++] = 0x06;\
  sdlStretcher[sdlStretcherPos++] = 0x83;\
  sdlStretcher[sdlStretcherPos++] = 0xc6;\
  sdlStretcher[sdlStretcherPos++] = 0x02;  

#define SDL_LOADL \
  sdlStretcher[sdlStretcherPos++] = 0x8b;\
  sdlStretcher[sdlStretcherPos++] = 0x06;\
  sdlStretcher[sdlStretcherPos++] = 0x83;\
  sdlStretcher[sdlStretcherPos++] = 0xc6;\
  sdlStretcher[sdlStretcherPos++] = 0x04;  

#define SDL_LOADL2 \
  sdlStretcher[sdlStretcherPos++] = 0x8b;\
  sdlStretcher[sdlStretcherPos++] = 0x06;\
  sdlStretcher[sdlStretcherPos++] = 0x83;\
  sdlStretcher[sdlStretcherPos++] = 0xc6;\
  sdlStretcher[sdlStretcherPos++] = 0x03;  

#define SDL_STOREW \
  sdlStretcher[sdlStretcherPos++] = 0x66;\
  sdlStretcher[sdlStretcherPos++] = 0x89;\
  sdlStretcher[sdlStretcherPos++] = 0x07;\
  sdlStretcher[sdlStretcherPos++] = 0x83;\
  sdlStretcher[sdlStretcherPos++] = 0xc7;\
  sdlStretcher[sdlStretcherPos++] = 0x02;  

#define SDL_STOREL \
  sdlStretcher[sdlStretcherPos++] = 0x89;\
  sdlStretcher[sdlStretcherPos++] = 0x07;\
  sdlStretcher[sdlStretcherPos++] = 0x83;\
  sdlStretcher[sdlStretcherPos++] = 0xc7;\
  sdlStretcher[sdlStretcherPos++] = 0x04;  

#define SDL_STOREL2 \
  sdlStretcher[sdlStretcherPos++] = 0x89;\
  sdlStretcher[sdlStretcherPos++] = 0x07;\
  sdlStretcher[sdlStretcherPos++] = 0x83;\
  sdlStretcher[sdlStretcherPos++] = 0xc7;\
  sdlStretcher[sdlStretcherPos++] = 0x03;  

#define SDL_RET \
  sdlStretcher[sdlStretcherPos++] = 0xc3;

#define SDL_PUSH_EAX \
  sdlStretcher[sdlStretcherPos++] = 0x50;

#define SDL_PUSH_ECX \
  sdlStretcher[sdlStretcherPos++] = 0x51;

#define SDL_PUSH_EBX \
  sdlStretcher[sdlStretcherPos++] = 0x53;

#define SDL_PUSH_ESI \
  sdlStretcher[sdlStretcherPos++] = 0x56;

#define SDL_PUSH_EDI \
  sdlStretcher[sdlStretcherPos++] = 0x57;

#define SDL_POP_EAX \
  sdlStretcher[sdlStretcherPos++] = 0x58;

#define SDL_POP_ECX \
  sdlStretcher[sdlStretcherPos++] = 0x59;

#define SDL_POP_EBX \
  sdlStretcher[sdlStretcherPos++] = 0x5b;

#define SDL_POP_ESI \
  sdlStretcher[sdlStretcherPos++] = 0x5e;

#define SDL_POP_EDI \
  sdlStretcher[sdlStretcherPos++] = 0x5f;

#define SDL_MOV_ECX(val) \
  sdlStretcher[sdlStretcherPos++] = 0xb9;\
  SDL_LONG(val);

#define SDL_REP_MOVSB \
  sdlStretcher[sdlStretcherPos++] = 0xf3;\
  sdlStretcher[sdlStretcherPos++] = 0xa4;

#define SDL_REP_MOVSW \
  sdlStretcher[sdlStretcherPos++] = 0xf3;\
  sdlStretcher[sdlStretcherPos++] = 0x66;\
  sdlStretcher[sdlStretcherPos++] = 0xa5;

#define SDL_REP_MOVSL \
  sdlStretcher[sdlStretcherPos++] = 0xf3;\
  sdlStretcher[sdlStretcherPos++] = 0xa5;

void sdlMakeStretcher(int width)
{
  switch(systemColorDepth) {
  case 16:
    if(sizeOption) {
      SDL_PUSH_EAX;
      SDL_PUSH_ESI;
      SDL_PUSH_EDI;
      for(int i = 0; i < width; i++) {
        SDL_LOADW;
        SDL_STOREW;
        SDL_STOREW;
        if(sizeOption > 1) {
          SDL_STOREW;
        }
        if(sizeOption > 2) {
          SDL_STOREW;
        }
      }
      SDL_POP_EDI;
      SDL_POP_ESI;
      SDL_POP_EAX;
      SDL_RET;
    } else {
      SDL_PUSH_ESI;
      SDL_PUSH_EDI;
      SDL_PUSH_ECX;
      SDL_MOV_ECX(width);
      SDL_REP_MOVSW;
      SDL_POP_ECX;
      SDL_POP_EDI;
      SDL_POP_ESI;
      SDL_RET;
    }
    break;
  case 24:
    if(sizeOption) {
      SDL_PUSH_EAX;
      SDL_PUSH_ESI;
      SDL_PUSH_EDI;
      int w = width - 1;
      for(int i = 0; i < w; i++) {
        SDL_LOADL2;
        SDL_STOREL2;
        SDL_STOREL2;
        if(sizeOption > 1) {
          SDL_STOREL2;
        }
        if(sizeOption > 2) {
          SDL_STOREL2;
        }
      }
      // need to write the last one
      SDL_LOADL2;
      SDL_STOREL2;
      if(sizeOption > 1) {
        SDL_STOREL2;
      }
      if(sizeOption > 2) {
        SDL_STOREL2;
      }
      SDL_AND_EAX(0x00ffffff);
      SDL_PUSH_EBX;
      SDL_LOADL_EBX;
      SDL_AND_EBX(0xff000000);
      SDL_OR_EAX_EBX;
      SDL_POP_EBX;
      SDL_STOREL2;
      SDL_POP_EDI;
      SDL_POP_ESI;
      SDL_POP_EAX;
      SDL_RET;
    } else {
      SDL_PUSH_ESI;
      SDL_PUSH_EDI;
      SDL_PUSH_ECX;
      SDL_MOV_ECX(3*width);
      SDL_REP_MOVSB;
      SDL_POP_ECX;
      SDL_POP_EDI;
      SDL_POP_ESI;
      SDL_RET;
    }
    break;
  case 32:
    if(sizeOption) {
      SDL_PUSH_EAX;
      SDL_PUSH_ESI;
      SDL_PUSH_EDI;
      for(int i = 0; i < width; i++) {
        SDL_LOADL;
        SDL_STOREL;
        SDL_STOREL;
        if(sizeOption > 1) {
          SDL_STOREL;
        }
        if(sizeOption > 2) {
          SDL_STOREL;
        }
      }
      SDL_POP_EDI;
      SDL_POP_ESI;
      SDL_POP_EAX;
      SDL_RET;
    } else {
      SDL_PUSH_ESI;
      SDL_PUSH_EDI;
      SDL_PUSH_ECX;
      SDL_MOV_ECX(width);
      SDL_REP_MOVSL;
      SDL_POP_ECX;
      SDL_POP_EDI;
      SDL_POP_ESI;
      SDL_RET;
    }
    break;
  }
}

#ifdef _MSC_VER
#define SDL_CALL_STRETCHER \
  {\
    __asm mov eax, stretcher\
    __asm mov edi, dest\
    __asm mov esi, src\
    __asm call eax\
  }
#else
#define SDL_CALL_STRETCHER \
        asm volatile("call *%%eax"::"a" (stretcher),"S" (src),"D" (dest))
#endif
#else
#define SDL_CALL_STRETCHER \
       sdlStretcher(src, dest)

void sdlStretch16x1(u8 *src, u8 *dest)
{
  u16 *s = (u16 *)src;
  u16 *d = (u16 *)dest;
  for(int i = 0; i < srcWidth; i++)
    *d++ = *s++;
}

void sdlStretch16x2(u8 *src, u8 *dest)
{
  u16 *s = (u16 *)src;
  u16 *d = (u16 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *s++;
  }
}

void sdlStretch16x3(u8 *src, u8 *dest)
{
  u16 *s = (u16 *)src;
  u16 *d = (u16 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *s;
    *d++ = *s++;
  }
}

void sdlStretch16x4(u8 *src, u8 *dest)
{
  u16 *s = (u16 *)src;
  u16 *d = (u16 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *s;
    *d++ = *s;
    *d++ = *s++;
  }
}

void (*sdlStretcher16[4])(u8 *, u8 *) = {
  sdlStretch16x1,
  sdlStretch16x2,
  sdlStretch16x3,
  sdlStretch16x4
};

void sdlStretch32x1(u8 *src, u8 *dest)
{
  u32 *s = (u32 *)src;
  u32 *d = (u32 *)dest;
  for(int i = 0; i < srcWidth; i++)
    *d++ = *s++;
}

void sdlStretch32x2(u8 *src, u8 *dest)
{
  u32 *s = (u32 *)src;
  u32 *d = (u32 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *s++;
  }
}

void sdlStretch32x3(u8 *src, u8 *dest)
{
  u32 *s = (u32 *)src;
  u32 *d = (u32 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *s;
    *d++ = *s++;
  }
}

void sdlStretch32x4(u8 *src, u8 *dest)
{
  u32 *s = (u32 *)src;
  u32 *d = (u32 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *s;
    *d++ = *s;
    *d++ = *s++;
  }
}

void (*sdlStretcher32[4])(u8 *, u8 *) = {
  sdlStretch32x1,
  sdlStretch32x2,
  sdlStretch32x3,
  sdlStretch32x4
};

void sdlStretch24x1(u8 *src, u8 *dest)
{
  u8 *s = src;
  u8 *d = dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s++;
    *d++ = *s++;
    *d++ = *s++;
  }
}

void sdlStretch24x2(u8 *src, u8 *dest)
{
  u8 *s = (u8 *)src;
  u8 *d = (u8 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
  }
}

void sdlStretch24x3(u8 *src, u8 *dest)
{
  u8 *s = (u8 *)src;
  u8 *d = (u8 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
  }
}

void sdlStretch24x4(u8 *src, u8 *dest)
{
  u8 *s = (u8 *)src;
  u8 *d = (u8 *)dest;
  for(int i = 0; i < srcWidth; i++) {
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
    *d++ = *s;
    *d++ = *(s+1);
    *d++ = *(s+2);
    s += 3;
  }
}

void (*sdlStretcher24[4])(u8 *, u8 *) = {
  sdlStretch24x1,
  sdlStretch24x2,
  sdlStretch24x3,
  sdlStretch24x4
};

#endif

u32 sdlFromHex(char *s)
{
  u32 value;
  sscanf(s, "%x", &value);
  return value;
}

#ifdef __MSC__
#define stat _stat
#define S_IFDIR _S_IFDIR
#endif

void sdlCheckDirectory(char *dir)
{
  struct stat buf;

  int len = strlen(dir);

  char *p = dir + len - 1;

  if(*p == '/' ||
     *p == '\\')
    *p = 0;
  
  if(stat(dir, &buf) == 0) {
    if(!(buf.st_mode & S_IFDIR)) {
      fprintf(stderr, "Error: %s is not a directory\n", dir);
      dir[0] = 0;
    }
  } else {
    fprintf(stderr, "Error: %s does not exist\n", dir);
    dir[0] = 0;
  }
}

char *sdlGetFilename(char *name)
{
  static char filebuffer[2048];

  int len = strlen(name);
  
  char *p = name + len - 1;
  
  while(true) {
    if(*p == '/' ||
       *p == '\\') {
      p++;
      break;
    }
    len--;
    p--;
    if(len == 0)
      break;
  }
  
  if(len == 0)
    strcpy(filebuffer, name);
  else
    strcpy(filebuffer, p);
  return filebuffer;
}

FILE *sdlFindPreferences()
{
  char buffer[4096];
  char path[2048];

#ifdef __GNUC__
#define PATH_SEP ":"
#define FILE_SEP '/'
#define EXE_NAME "VisualBoyAdvance"
#else
#define PATH_SEP ";"
#define FILE_SEP '\\'
#define EXE_NAME "VisualBoyAdvance-SDL.exe"
#endif
  
  if(GETCWD(buffer, 2048)) {
    fprintf(stderr, "Searching for configuration file at: %s\n", buffer);
  }
  
  FILE *f = fopen("VisualBoyAdvance.cfg","r");

  if(f != NULL) {
    return f;
  }

  char *home = getenv("HOME");

  if(home != NULL) {
    fprintf(stderr, "Seaching home directory: %s\n", home);
    sprintf(path, "%s%cVisualBoyAdvance.cfg", home, FILE_SEP);
    f = fopen(path, "r");
    
    if(f)
      return f;
  }

#ifdef _MSC_VER
  home = getenv("USERPROFILE");
  if(home != NULL) {
    fprintf(stderr, "Searching user profile directory: %s\n", home);
    sprintf(path, "%s%cVisualBoyAdvance.cfg", home, FILE_SEP);
    f = fopen(path, "r");
    if(f)
      return f;
  }
#endif
  
  if(!strchr(arg0, '/') &&
     !strchr(arg0, '\\')) {
    char *path = getenv("PATH");

    if(path != NULL) {
      fprintf(stderr, "Searching PATH\n");
      strncpy(buffer, path, 4096);
      buffer[4095] = 0;
      char *tok = strtok(buffer, PATH_SEP);
      
      while(tok) {
        sprintf(path, "%s%c%s", tok, FILE_SEP, EXE_NAME);
        f = fopen(path, "r");
        if(f != NULL) {
          char path2[2048];
          fclose(f);
          sprintf(path2, "%s%cVisualBoyAdvance.cfg", tok, FILE_SEP);
          f = fopen(path2, "r");
          if(f) {
            fprintf(stderr, "Found at %s\n", path2);
            return f;
          }
        }
        tok = strtok(NULL, PATH_SEP);
      }
    }
  } else {
    // executable is relative to some directory
    fprintf(stderr, "Searching executable directory\n");
    strcpy(buffer, arg0);
    char *p = strrchr(buffer, FILE_SEP);
    if(p) {
      *p = 0;
      sprintf(path, "%s%cVisualBoyAdvance.cfg", buffer, FILE_SEP);
      f = fopen(path, "r");
      if(f)
        return f;
    }
  }
  return NULL;
}

void sdlReadPreferences(FILE *f)
{
  char buffer[2048];
  
  while(1) {
    char *s = fgets(buffer, 2048, f);

    if(s == NULL)
      break;

    char *p  = strchr(s, '#');
    
    if(p)
      *p = 0;
    
    char *token = strtok(s, " \t\n\r=");

    if(!token)
      continue;

    if(strlen(token) == 0)
      continue;

    char *key = token;
    char *value = strtok(NULL, "\t\n\r");

    if(value == NULL) {
      fprintf(stderr, "Empty value for key %s\n", key);
      continue;
    }

    if(!strcmp(key,"Joy0_Left")) {
      joypad[0][KEY_LEFT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Right")) {
      joypad[0][KEY_RIGHT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Up")) {
      joypad[0][KEY_UP] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Down")) {
      joypad[0][KEY_DOWN] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_A")) {
      joypad[0][KEY_BUTTON_A] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_B")) {
      joypad[0][KEY_BUTTON_B] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_L")) {
      joypad[0][KEY_BUTTON_L] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_R")) {
      joypad[0][KEY_BUTTON_R] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Start")) {
      joypad[0][KEY_BUTTON_START] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Select")) {
      joypad[0][KEY_BUTTON_SELECT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Speed")) {
      joypad[0][KEY_BUTTON_SPEED] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy0_Capture")) {
      joypad[0][KEY_BUTTON_CAPTURE] = sdlFromHex(value);
    } else if(!strcmp(key,"Joy1_Left")) {
      joypad[1][KEY_LEFT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Right")) {
      joypad[1][KEY_RIGHT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Up")) {
      joypad[1][KEY_UP] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Down")) {
      joypad[1][KEY_DOWN] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_A")) {
      joypad[1][KEY_BUTTON_A] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_B")) {
      joypad[1][KEY_BUTTON_B] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_L")) {
      joypad[1][KEY_BUTTON_L] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_R")) {
      joypad[1][KEY_BUTTON_R] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Start")) {
      joypad[1][KEY_BUTTON_START] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Select")) {
      joypad[1][KEY_BUTTON_SELECT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Speed")) {
      joypad[1][KEY_BUTTON_SPEED] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy1_Capture")) {
      joypad[1][KEY_BUTTON_CAPTURE] = sdlFromHex(value);
    } else if(!strcmp(key,"Joy2_Left")) {
      joypad[2][KEY_LEFT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Right")) {
      joypad[2][KEY_RIGHT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Up")) {
      joypad[2][KEY_UP] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Down")) {
      joypad[2][KEY_DOWN] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_A")) {
      joypad[2][KEY_BUTTON_A] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_B")) {
      joypad[2][KEY_BUTTON_B] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_L")) {
      joypad[2][KEY_BUTTON_L] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_R")) {
      joypad[2][KEY_BUTTON_R] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Start")) {
      joypad[2][KEY_BUTTON_START] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Select")) {
      joypad[2][KEY_BUTTON_SELECT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Speed")) {
      joypad[2][KEY_BUTTON_SPEED] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy2_Capture")) {
      joypad[2][KEY_BUTTON_CAPTURE] = sdlFromHex(value);
    } else if(!strcmp(key,"Joy4_Left")) {
      joypad[4][KEY_LEFT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Right")) {
      joypad[4][KEY_RIGHT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Up")) {
      joypad[4][KEY_UP] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Down")) {
      joypad[4][KEY_DOWN] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_A")) {
      joypad[4][KEY_BUTTON_A] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_B")) {
      joypad[4][KEY_BUTTON_B] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_L")) {
      joypad[4][KEY_BUTTON_L] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_R")) {
      joypad[4][KEY_BUTTON_R] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Start")) {
      joypad[4][KEY_BUTTON_START] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Select")) {
      joypad[4][KEY_BUTTON_SELECT] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Speed")) {
      joypad[4][KEY_BUTTON_SPEED] = sdlFromHex(value);
    } else if(!strcmp(key, "Joy4_Capture")) {
      joypad[4][KEY_BUTTON_CAPTURE] = sdlFromHex(value);
    } else if(!strcmp(key, "Motion_Left")) {
      motion[KEY_LEFT] = sdlFromHex(value);
    } else if(!strcmp(key, "Motion_Right")) {
      motion[KEY_RIGHT] = sdlFromHex(value);
    } else if(!strcmp(key, "Motion_Up")) {
      motion[KEY_UP] = sdlFromHex(value);
    } else if(!strcmp(key, "Motion_Down")) {
      motion[KEY_DOWN] = sdlFromHex(value);
    } else if(!strcmp(key, "frameSkip")) {
      frameSkip = sdlFromHex(value);
      if(frameSkip < 0 || frameSkip > 9)
        frameSkip = 2;
    } else if(!strcmp(key, "gbFrameSkip")) {
      gbFrameSkip = sdlFromHex(value);
      if(gbFrameSkip < 0 || gbFrameSkip > 9)
        gbFrameSkip = 0;      
    } else if(!strcmp(key, "video")) {
      sizeOption = sdlFromHex(value);
      if(sizeOption < 0 || sizeOption > 3)
        sizeOption = 1;
    } else if(!strcmp(key, "fullScreen")) {
      fullscreen = sdlFromHex(value) ? 1 : 0;
    } else if(!strcmp(key, "useBios")) {
      useBios = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "biosFile")) {
      strcpy(biosFileName, value);
    } else if(!strcmp(key, "filter")) {
      filter = sdlFromHex(value);
      if(filter < 0 || filter > 10)
        filter = 0;
    } else if(!strcmp(key, "disableStatus")) {
      disableStatusMessages = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "borderOn")) {
      gbBorderOn = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "emulatorType")) {
      gbEmulatorType = sdlFromHex(value);
      if(gbEmulatorType < 0 || gbEmulatorType > 4)
        gbEmulatorType = 1;
    } else if(!strcmp(key, "colorOption")) {
      gbColorOption = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "captureDir")) {
      sdlCheckDirectory(value);
      strcpy(captureDir, value);
    } else if(!strcmp(key, "saveDir")) {
      sdlCheckDirectory(value);
      strcpy(saveDir, value);
    } else if(!strcmp(key, "batteryDir")) {
      sdlCheckDirectory(value);
      strcpy(batteryDir, value);
    } else if(!strcmp(key, "captureFormat")) {
      captureFormat = sdlFromHex(value);
    } else if(!strcmp(key, "soundQuality")) {
      soundQuality = sdlFromHex(value);
      switch(soundQuality) {
      case 1:
      case 2:
      case 4:
        break;
      default:
        fprintf(stderr, "Unknown sound quality %d. Defaulting to 22Khz\n", 
soundQuality);
        soundQuality = 2;
        break;
      }
    } else if(!strcmp(key, "soundEcho")) {
      soundEcho = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "soundLowPass")) {
      soundLowPass = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "soundReverse")) {
      soundReverse = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "soundVolume")) {
      soundVolume = sdlFromHex(value);
      if(soundVolume < 0 || soundVolume > 3)
        soundVolume = 0;
    } else if(!strcmp(key, "removeIntros")) {
      removeIntros = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "saveType")) {
      cpuSaveType = sdlFromHex(value);
      if(cpuSaveType < 0 || cpuSaveType > 4)
        cpuSaveType = 0;
    } else if(!strcmp(key, "flashSize")) {
      sdlFlashSize = sdlFromHex(value);
      if(sdlFlashSize != 0 && sdlFlashSize != 1)
        sdlFlashSize = 0;
    } else if(!strcmp(key, "ifbType")) {
      ifbType = sdlFromHex(value);
      if(ifbType < 0 || ifbType > 2)
        ifbType = 0;
    } else if(!strcmp(key, "showSpeed")) {
      showSpeed = sdlFromHex(value);
      if(showSpeed < 0 || showSpeed > 2)
        showSpeed = 1;
    } else if(!strcmp(key, "showSpeedTransparent")) {
      showSpeedTransparent = sdlFromHex(value);
    } else if(!strcmp(key, "autoFrameSkip")) {
      autoFrameSkip = sdlFromHex(value);
    } else if(!strcmp(key, "throttle")) {
      throttle = sdlFromHex(value);
      if(throttle != 0 && (throttle < 5 || throttle > 1000))
        throttle = 0;
    } else if(!strcmp(key, "disableMMX")) {
#ifdef MMX
      cpu_mmx = sdlFromHex(value) ? false : true;
#endif
    } else {
      fprintf(stderr, "Unknown configuration key %s\n", key);
    }
  }
}

void sdlReadPreferences()
{
  FILE *f = sdlFindPreferences();

  if(f == NULL) {
    fprintf(stderr, "Configuration file NOT FOUND (using defaults)\n");
    return;
  } else
    fprintf(stderr, "Reading configuration file.\n");

  sdlReadPreferences(f);

  fclose(f);
}

int sdlCalculateShift(u32 mask)
{
  int m = 0;
  
  while(mask) {
    m++;
    mask >>= 1;
  }

  return m-5;
}

int sdlCalculateMaskWidth(u32 mask)
{
  int m = 0;
  int mask2 = mask;

  while(mask2) {
    m++;
    mask2 >>= 1;
  }

  int m2 = 0;
  mask2 = mask;
  while(!(mask2 & 1)) {
    m2++;
    mask2 >>= 1;
  }

  return m - m2;
}

void sdlWriteState(int num)
{
  char stateName[2048];

  if(saveDir[0])
    sprintf(stateName, "%s/%s%d.sgm", saveDir, sdlGetFilename(filename),
            num+1);
  else
    sprintf(stateName,"%s%d.sgm", filename, num+1);
  if(emuWriteState)
    emuWriteState(stateName);
  sprintf(stateName, "Wrote state %d", num+1);
  systemScreenMessage(stateName);
}

void sdlReadState(int num)
{
  char stateName[2048];

  if(saveDir[0])
    sprintf(stateName, "%s/%s%d.sgm", saveDir, sdlGetFilename(filename),
            num+1);
  else
    sprintf(stateName,"%s%d.sgm", filename, num+1);

  if(emuReadState)
    emuReadState(stateName);

  sprintf(stateName, "Loaded state %d", num+1);
  systemScreenMessage(stateName);
}

void sdlWriteBattery()
{
  char buffer[1048];

  if(batteryDir[0])
    sprintf(buffer, "%s/%s.sav", batteryDir, sdlGetFilename(filename));
  else  
    sprintf(buffer, "%s.sav", filename);

  emuWriteBattery(buffer);

  systemScreenMessage("Wrote battery");
}

void sdlReadBattery()
{
  char buffer[1048];
  
  if(batteryDir[0])
    sprintf(buffer, "%s/%s.sav", batteryDir, sdlGetFilename(filename));
  else 
    sprintf(buffer, "%s.sav", filename);
  
  bool res = false;

  res = emuReadBattery(buffer);

  if(res)
    systemScreenMessage("Loaded battery");
}

#define MOD_KEYS    (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_META)
#define MOD_NOCTRL  (KMOD_SHIFT|KMOD_ALT|KMOD_META)
#define MOD_NOALT   (KMOD_CTRL|KMOD_SHIFT|KMOD_META)
#define MOD_NOSHIFT (KMOD_CTRL|KMOD_ALT|KMOD_META)

void sdlUpdateKey(int key, bool down)
{
  int i;
  for(int j = 0; j < 4; j++) {
    for(i = 0 ; i < 12; i++) {
      if((joypad[j][i] & 0xf000) == 0) {
        if(key == joypad[j][i])
          sdlButtons[j][i] = down;
      }
    }
  }
  for(i = 0 ; i < 4; i++) {
    if((motion[i] & 0xf000) == 0) {
      if(key == motion[i])
        sdlMotionButtons[i] = down;
    }
  }
}

void sdlUpdateJoyButton(int which,
                        int button,
                        bool pressed)
{
  int i;
  for(int j = 0; j < 4; j++) {
    for(i = 0; i < 12; i++) {
      int dev = (joypad[j][i] >> 12);
      int b = joypad[j][i] & 0xfff;
      if(dev) {
        dev--;
        
        if((dev == which) && (b >= 128) && (b == (button+128))) {
          sdlButtons[j][i] = pressed;
        }
      }
    }
  }
  for(i = 0; i < 4; i++) {
    int dev = (motion[i] >> 12);
    int b = motion[i] & 0xfff;
    if(dev) {
      dev--;

      if((dev == which) && (b >= 128) && (b == (button+128))) {
        sdlMotionButtons[i] = pressed;
      }
    }
  }  
}

void sdlUpdateJoyHat(int which,
                     int hat,
                     int value)
{
  int i;
  for(int j = 0; j < 4; j++) {
    for(i = 0; i < 12; i++) {
      int dev = (joypad[j][i] >> 12);
      int a = joypad[j][i] & 0xfff;
      if(dev) {
        dev--;
        
        if((dev == which) && (a>=32) && (a < 48) && (((a&15)>>2) == hat)) {
          int dir = a & 3;
          int v = 0;
          switch(dir) {
          case 0:
            v = value & SDL_HAT_UP;
            break;
          case 1:
            v = value & SDL_HAT_DOWN;
            break;
          case 2:
            v = value & SDL_HAT_RIGHT;
            break;
          case 3:
            v = value & SDL_HAT_LEFT;
            break;
          }
          sdlButtons[j][i] = (v ? true : false);
        }
      }
    }
  }
  for(i = 0; i < 4; i++) {
    int dev = (motion[i] >> 12);
    int a = motion[i] & 0xfff;
    if(dev) {
      dev--;

      if((dev == which) && (a>=32) && (a < 48) && (((a&15)>>2) == hat)) {
        int dir = a & 3;
        int v = 0;
        switch(dir) {
        case 0:
          v = value & SDL_HAT_UP;
          break;
        case 1:
          v = value & SDL_HAT_DOWN;
          break;
        case 2:
          v = value & SDL_HAT_RIGHT;
          break;
        case 3:
          v = value & SDL_HAT_LEFT;
          break;
        }
        sdlMotionButtons[i] = (v ? true : false);
      }
    }
  }      
}

void sdlUpdateJoyAxis(int which,
                      int axis,
                      int value)
{
  int i;
  for(int j = 0; j < 4; j++) {
    for(i = 0; i < 12; i++) {
      int dev = (joypad[j][i] >> 12);
      int a = joypad[j][i] & 0xfff;
      if(dev) {
        dev--;
        
        if((dev == which) && (a < 32) && ((a>>1) == axis)) {
          sdlButtons[j][i] = (a & 1) ? (value > 16384) : (value < -16384);
        }
      }
    }
  }
  for(i = 0; i < 4; i++) {
    int dev = (motion[i] >> 12);
    int a = motion[i] & 0xfff;
    if(dev) {
      dev--;

      if((dev == which) && (a < 32) && ((a>>1) == axis)) {
        sdlMotionButtons[i] = (a & 1) ? (value > 16384) : (value < -16384);
      }
    }
  }  
}

bool sdlCheckJoyKey(int key)
{
  int dev = (key >> 12) - 1;
  int what = key & 0xfff;

  if(what >= 128) {
    // joystick button
    int button = what - 128;

    if(button >= SDL_JoystickNumButtons(sdlDevices[dev]))
      return false;
  } else if (what < 0x20) {
    // joystick axis    
    what >>= 1;
    if(what >= SDL_JoystickNumAxes(sdlDevices[dev]))
      return false;
  } else if (what < 0x30) {
    // joystick hat
    what = (what & 15);
    what >>= 2;
    if(what >= SDL_JoystickNumHats(sdlDevices[dev]))
      return false;
  }

  // no problem found
  return true;
}

void sdlCheckKeys()
{
  sdlNumDevices = SDL_NumJoysticks();

  if(sdlNumDevices)
    sdlDevices = (SDL_Joystick **)calloc(1,sdlNumDevices *
                                         sizeof(SDL_Joystick **));
  int i;

  bool usesJoy = false;

  for(int j = 0; j < 4; j++) {
    for(i = 0; i < 12; i++) {
      int dev = joypad[j][i] >> 12;
      if(dev) {
        dev--;
        bool ok = false;
        
        if(sdlDevices) {
          if(dev < sdlNumDevices) {
            if(sdlDevices[dev] == NULL) {
              sdlDevices[dev] = SDL_JoystickOpen(dev);
            }
            
            ok = sdlCheckJoyKey(joypad[j][i]);
          } else
            ok = false;
        }
        
        if(!ok)
          joypad[j][i] = defaultJoypad[i];
        else
          usesJoy = true;
      }
    }
  }

  for(i = 0; i < 4; i++) {
    int dev = motion[i] >> 12;
    if(dev) {
      dev--;
      bool ok = false;
      
      if(sdlDevices) {
        if(dev < sdlNumDevices) {
          if(sdlDevices[dev] == NULL) {
            sdlDevices[dev] = SDL_JoystickOpen(dev);
          }
          
          ok = sdlCheckJoyKey(motion[i]);
        } else
          ok = false;
      }
      
      if(!ok)
        motion[i] = defaultMotion[i];
      else
        usesJoy = true;
    }
  }

  if(usesJoy)
    SDL_JoystickEventState(SDL_ENABLE);
}

void sdlPollEvents()
{
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    switch(event.type) {
    case SDL_MOUSEMOTION:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN:
      if(fullscreen) {
        SDL_ShowCursor(SDL_ENABLE);
        mouseCounter = 120;
      }
      break;
    case SDL_JOYHATMOTION:
      sdlUpdateJoyHat(event.jhat.which,
                      event.jhat.hat,
                      event.jhat.value);
      break;
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
      sdlUpdateJoyButton(event.jbutton.which,
                         event.jbutton.button,
                         event.jbutton.state == SDL_PRESSED);
      break;
    case SDL_JOYAXISMOTION:
      sdlUpdateJoyAxis(event.jaxis.which,
                       event.jaxis.axis,
                       event.jaxis.value);
      break;
    case SDL_KEYDOWN:
      sdlUpdateKey(event.key.keysym.sym, true);
      break;
    case SDL_KEYUP:
      switch(event.key.keysym.sym) {
      case SDLK_r:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          if(emulating) {
            emuReset();

            systemScreenMessage("Reset");
          }
        }
        break;
      case SDLK_p:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          paused = !paused;
          SDL_PauseAudio(paused);
          if(paused)
            wasPaused = true;
        }
        break;
      case SDLK_ESCAPE:
        emulating = 0;
        break;
      case SDLK_f:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          int flags = 0;
          fullscreen = !fullscreen;
          if(fullscreen)
            flags |= SDL_FULLSCREEN;
          SDL_SetVideoMode(destWidth, destHeight, systemColorDepth, flags);
          //          if(SDL_WM_ToggleFullScreen(surface))
          //            fullscreen = !fullscreen;
        }
        break;
      case SDLK_F11:
        if(dbgMain != debuggerMain) {
          if(armState) {
            armNextPC -= 4;
            reg[15].I -= 4;
          } else {
            armNextPC -= 2;
            reg[15].I -= 2;
          }
        }
        debugger = true;
        break;
      case SDLK_F1:
      case SDLK_F2:
      case SDLK_F3:
      case SDLK_F4:
      case SDLK_F5:
      case SDLK_F6:
      case SDLK_F7:
      case SDLK_F8:
      case SDLK_F9:
      case SDLK_F10:
        if(!(event.key.keysym.mod & MOD_NOSHIFT) &&
           (event.key.keysym.mod & KMOD_SHIFT)) {
          sdlWriteState(event.key.keysym.sym-SDLK_F1);
        } else if(!(event.key.keysym.mod & MOD_KEYS)) {
          sdlReadState(event.key.keysym.sym-SDLK_F1);
        }
        break;
      case SDLK_1:
      case SDLK_2:
      case SDLK_3:
      case SDLK_4:
        if(!(event.key.keysym.mod & MOD_NOALT) &&
           (event.key.keysym.mod & KMOD_ALT)) {
          char *disableMessages[4] = 
            { "autofire A disabled",
              "autofire B disabled",
              "autofire R disabled",
              "autofire L disabled"};
          char *enableMessages[4] = 
            { "autofire A",
              "autofire B",
              "autofire R",
              "autofire L"};
          int mask = 1 << (event.key.keysym.sym - SDLK_1);
    if(event.key.keysym.sym > SDLK_2)
      mask <<= 6;
          if(autoFire & mask) {
            autoFire &= ~mask;
            systemScreenMessage(disableMessages[event.key.keysym.sym - SDLK_1]);
          } else {
            autoFire |= mask;
            systemScreenMessage(enableMessages[event.key.keysym.sym - SDLK_1]);
          }
        } if(!(event.key.keysym.mod & MOD_NOCTRL) &&
             (event.key.keysym.mod & KMOD_CTRL)) {
          int mask = 0x0100 << (event.key.keysym.sym - SDLK_1);
          layerSettings ^= mask;
          layerEnable = DISPCNT & layerSettings;
        }
        break;
      case SDLK_5:
      case SDLK_6:
      case SDLK_7:
      case SDLK_8:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          int mask = 0x0100 << (event.key.keysym.sym - SDLK_1);
          layerSettings ^= mask;
          layerEnable = DISPCNT & layerSettings;
        }
        break;
      case SDLK_n:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          if(paused)
            paused = false;
          pauseNextFrame = true;
        }
        break;
      }
      sdlUpdateKey(event.key.keysym.sym, false);
      break;
    }
  }
}

void usage(char *cmd)
{
        printf("%s [options] file-name\n",cmd);
        printf("  options:\n");
        printf("  -1 , --video-1x             1x\n");
        printf("  -2 , --video-2x             2x\n");
        printf("  -3 , --video-3x             3x\n");
        printf("  -4 , --video 4x             4x\n");        
        printf("  -F , --fullscreen           Full screen\n");
        printf("  -G , --gdb==PROTOCOL        GNU Remote Stub mode:\n");
        printf("                               tcp      - use TCP at port 55555\n");
        printf("                               tcp:PORT - use TCP at port PORT\n");
        printf("                               pipe     - use pipe transport\n");
        printf("  -N , --no-debug             Don't parse debug information\n");
        printf("  -S , --flash-size=SIZE      Set the Flash size\n");
        printf("       --flash-64k             0 -  64K Flash\n");
        printf("       --flash-128k            1 - 128K Flash\n");
        printf("  -T , --throttle=THROTTLE    Set the desired throttle (5...1000)\n");
        printf("  -Y , --yuv=TYPE             Use YUV overlay for drawing:\n");
        printf("                               0 - YV12\n");
        printf("                               1 - UYVY\n");
        printf("                               2 - YVYU\n");
        printf("                               3 - YUY2\n");
        printf("                               4 - IYUV\n");
        printf("  -b , --bios=BIOS            Use given bios file\n");
        printf("  -c,  --config=FILE          Read the given configuration file\n");
        printf("  -d , --debug                Enter debugger\n");        
        printf("  -f , --filter=FILTER        Select filter:\n");
        printf("       --filter-normal         0 - normal mode\n");
        printf("       --filter-tv-mode        1 - TV Mode\n");
        printf("       --filter-2xsai          2 - 2xSaI\n");
        printf("       --filter-super-2xsai    3 - Super 2xSaI\n");
        printf("       --filter-super-eagle    4 - Super Eagle\n");
        printf("       --filter-pixelate       5 - Pixelate\n");
        printf("       --filter-motion-blur    6 - Motion Blur\n");
        printf("       --filter-advmame        7 - AdvanceMAME Scale2x\n");
        printf("       --filter-simple2x       8 - Simple2x\n");
        printf("       --filter-bilinear       9 - Bilinear\n");
        printf("       --filter-bilinear+     10 - Bilinear Plus\n");        
        printf("  -h , --help                 Print this help\n");
        printf("  -i , --ips=PATCH            Apply given IPS patch\n");
        printf("  -p , --profile=[HERTZ]      Enable profiling\n");
        printf("  -s , --frameskip=FRAMESKIP  Set frame skip (0...9)\n");
        printf("  -t , --save-type=TYPE       Set the available save type\n");
        printf("       --save-auto             0 - Automatic (EEPROM, SRAM, FLASH)\n");
        printf("       --save-eeprom           1 - EEPROM\n");
        printf("       --save-sram             2 - SRAM\n");
        printf("       --save-flash            3 - FLASH\n");
        printf("       --save-sensor           4 - EEPROM+Sensor\n");
        printf("  -v , --verbose=VERBOSE      Set verbose logging (trace.log)\n");
        printf("                                 1 - SWI\n");
        printf("                                 2 - Unaligned memory access\n");
        printf("                                 4 - Illegal memory write\n");
        printf("                                 8 - Illegal memory read\n");
        printf("                                16 - DMA 0\n");
        printf("                                32 - DMA 1\n");
        printf("                                64 - DMA 2\n");
        printf("                               128 - DMA 3\n");
        printf("\n");
        printf("Long options only:\n");
        printf("       --auto-frameskip       Enable auto frameskipping\n");
        printf("       --ifb-none             No interframe blending\n");
        printf("       --ifb-motion-blur      Interframe motion blur\n");
        printf("       --ifb-smart            Smart interframe blending\n");
        printf("       --no-auto-frameskip    Disable auto frameskipping\n");
        printf("       --no-ips               Do not apply IPS patch\n");
        printf("       --no-mmx               Disable MMX support\n");
        printf("       --no-show-speed        Don't show emulation speed\n");
        printf("       --no-throttle          Disable thrrotle\n");
        printf("       --show-speed-normal    Show emulation speed\n");
        printf("       --show-speed-detailed  Show detailed speed data\n");
}

int main(int argc, char **argv)
{
  fprintf(stderr,"VisualBoyAdvance-SDL version %s\n", VERSION);
#ifdef __GNUC__
  fprintf(stderr,"Linux version\n");
#else
  fprintf(stderr,"Windows version\n");
#endif
  arg0 = argv[0];
  
  captureDir[0] = 0;
  saveDir[0] = 0;
  batteryDir[0] = 0;
  ipsname[0] = 0;
  
  char buffer[1024];

  int op = -1;

  frameSkip = 2;
  gbBorderOn = 0;

  parseDebug = true;

  sdlReadPreferences();

  sdlPrintUsage = 0;
  
  while((op = getopt_long(argc,
                          argv,
                          "FNT:Y:G:D:b:c:df:hi:p::s:t:v:1234",
                          sdlOptions,
                          NULL)) != -1) {
    switch(op) {
    case 0:
      // long option already processed by getopt_long
      break;
    case 'b':
      useBios = true;
      if(optarg == NULL) {
        fprintf(stderr, "Missing BIOS file name\n");
        exit(-1);
      }
      strcpy(biosFileName, optarg);
      break;
    case 'c':
      {
        if(optarg == NULL) {
          fprintf(stderr, "Missing config file name\n");
          exit(-1);
        }
        FILE *f = fopen(optarg, "r");
        if(f == NULL) {
          fprintf(stderr, "File not found %s\n", optarg);
          exit(-1);
        }
        sdlReadPreferences(f);
        fclose(f);
      }
      break;
    case 'd':
      debugger = true;
      break;
    case 'h':
      sdlPrintUsage = 1;
      break;
    case 'i':
      if(optarg == NULL) {
        fprintf(stderr, "Missing IPS name\n");
        exit(-1);
        strcpy(ipsname, optarg);
      }
      break;
    case 'Y':
      yuv = true;
      if(optarg) {
        yuvType = atoi(optarg);
        switch(yuvType) {
        case 0:
          yuvType = SDL_YV12_OVERLAY;
          break;
        case 1:
          yuvType = SDL_UYVY_OVERLAY;
          break;
        case 2:
          yuvType = SDL_YVYU_OVERLAY;
          break;
        case 3:
          yuvType = SDL_YUY2_OVERLAY;
          break;
        case 4:
          yuvType = SDL_IYUV_OVERLAY;
          break;
        default:
          yuvType = SDL_YV12_OVERLAY;
        }
      } else
        yuvType = SDL_YV12_OVERLAY;
      break;
    case 'G':
      dbgMain = remoteStubMain;
      dbgSignal = remoteStubSignal;
      dbgOutput = remoteOutput;
      debugger = true;
      debuggerStub = true;
      if(optarg) {
        char *s = optarg;
        if(strncmp(s,"tcp:", 4) == 0) {
          s+=4;
          int port = atoi(s);
          remoteSetProtocol(0);
          remoteSetPort(port);
        } else if(strcmp(s,"tcp") == 0) {
          remoteSetProtocol(0);
        } else if(strcmp(s, "pipe") == 0) {
          remoteSetProtocol(1);
        } else {
          fprintf(stderr, "Unknown protocol %s\n", s);
          exit(-1);
        }
      } else {
        remoteSetProtocol(0);
      }
      break;
    case 'N':
      parseDebug = false;
      break;
    case 'D':
      if(optarg) {
        systemDebug = atoi(optarg);
      } else {
        systemDebug = 1;
      }
      break;
    case 'F':
      fullscreen = 1;
      mouseCounter = 120;
      break;
    case 'f':
      if(optarg) {
        filter = atoi(optarg);
      } else {
        filter = 0;
      }
      break;
    case 'p':
#ifdef PROFILING
      if(optarg) {
        cpuEnableProfiling(atoi(optarg));
      } else
        cpuEnableProfiling(100);
#endif
      break;
    case 'S':
      sdlFlashSize = atoi(optarg);
      if(sdlFlashSize < 0 || sdlFlashSize > 1)
        sdlFlashSize = 0;
      break;
    case 's':
      if(optarg) {
        int a = atoi(optarg);
        if(a >= 0 && a <= 9) {
          gbFrameSkip = a;
          frameSkip = a;
        }
      } else {
        frameSkip = 2;
        gbFrameSkip = 0;
      }
      break;
    case 't':
      if(optarg) {
        int a = atoi(optarg);
        if(a < 0 || a > 4)
          a = 0;
        cpuSaveType = a;
      }
      break;
    case 'T':
      if(optarg) {
        int t = atoi(optarg);
        if(t < 5 || t > 1000)
          t = 0;
        throttle = t;
      }
      break;
    case 'v':
      if(optarg) {
        systemVerbose = atoi(optarg);
      } else 
        systemVerbose = 0;
      break;
    case '1':
      sizeOption = 0;
      break;
    case '2':
      sizeOption = 1;
      break;
    case '3':
      sizeOption = 2;
      break;
    case '4':
      sizeOption = 3;
      break;
    case '?':
      sdlPrintUsage = 1;
      break;
    }
  }

  if(sdlPrintUsage) {
    usage(argv[0]);
    exit(-1);
  }

#ifdef MMX
  if(disableMMX)
    cpu_mmx = 0;
#endif

  if(sdlFlashSize == 0)
    flashSetSize(0x10000);
  else
    flashSetSize(0x20000);
  
  if(!debuggerStub) {
    if(optind >= argc) {
      systemMessage(0,"Missing image name");
      usage(argv[0]);
      exit(-1);
    }
  }

  if(filter) {
    sizeOption = 1;
  }

  for(int i = 0; i < 24;) {
    systemGbPalette[i++] = (0x1f) | (0x1f << 5) | (0x1f << 10);
    systemGbPalette[i++] = (0x15) | (0x15 << 5) | (0x15 << 10);
    systemGbPalette[i++] = (0x0c) | (0x0c << 5) | (0x0c << 10);
    systemGbPalette[i++] = 0;
  }

  if(optind < argc) {
    char *szFile = argv[optind];

    strcpy(filename, szFile);
    char *p = strrchr(filename, '.');

    if(p)
      *p = 0;

    if(ipsname[0] == 0)
      sprintf(ipsname, "%s.ips", filename);
    
    bool failed = false;
    if(CPUIsZipFile(szFile)) {
      unzFile unz = unzOpen(szFile);
      
      if(unz == NULL) {
        systemMessage(0, "Cannot open file %s", szFile);
        exit(-1);
      }
      int r = unzGoToFirstFile(unz);
      
      if(r != UNZ_OK) {
        unzClose(unz);
        systemMessage(0, "Bad ZIP file %s", szFile);
        exit(-1);
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
          systemMessage(0,"Bad ZIP file %s", szFile);
          exit(-1);
        }
        
        if(gbIsGameboyRom(buffer)) {
          found = true;
          cartridgeType = 1;
          break;
        }
        if(CPUIsGBAImage(buffer)) {
          found = true;
          cartridgeType = 0;
          break;
        }
        
        r = unzGoToNextFile(unz);
        
        if(r != UNZ_OK)
          break;
      }
      
      if(!found) {
        unzClose(unz);
        systemMessage(0, "No image found on ZIP file %s", szFile);
        exit(-1);
      }
      
      unzClose(unz);
    }
    
    if(gbIsGameboyRom(szFile) || cartridgeType == 1) {
      failed = !gbLoadRom(szFile);
      if(!failed) {
        cartridgeType = 1;
        emuWriteState = gbWriteSaveState;
        emuReadState = gbReadSaveState;
        emuWriteBattery = gbWriteBatteryFile;
        emuReadBattery = gbReadBatteryFile;
        emuReset = gbReset;
        emuCleanUp = gbCleanUp;
        emuWritePNG = gbWritePNGFile;
        emuWriteBMP = gbWriteBMPFile;
        emuMain = gbEmulate;
        emuUpdateCPSR = NULL;
        emuHasDebugger = false;
        emuCount = 70000/4;
        if(sdlAutoIPS) {
          int size = gbRomSize;
          utilApplyIPS(ipsname, &gbRom, &size);
          if(size != gbRomSize) {
            extern bool gbUpdateSizes();
            gbUpdateSizes();
            gbReset();
          }
        }
      }
    } else if(CPUIsGBAImage(szFile) || cartridgeType == 0) {
      failed = !CPULoadRom(szFile);
      if(!failed) {
        cartridgeType = 0;
        emuWriteState = CPUWriteState;
        emuReadState = CPUReadState;
        emuWriteBattery = CPUWriteBatteryFile;
        emuReadBattery = CPUReadBatteryFile;
        emuReset = CPUReset;
        emuCleanUp = CPUCleanUp;
        emuWritePNG = CPUWritePNGFile;
        emuWriteBMP = CPUWriteBMPFile;
        emuMain = CPULoop;
        emuUpdateCPSR = CPUUpdateCPSR;
        emuHasDebugger = true;
        emuCount = 50000;

        if(removeIntros && rom != NULL) {
          *((u32 *)rom)= TO32LE(0xea00002e);
        }
        
        CPUInit(biosFileName, useBios);
        CPUReset();
        if(sdlAutoIPS) {
          int size = 0x2000000;
          utilApplyIPS(ipsname, &rom, &size);
          if(size != 0x2000000) {
            CPUReset();
          }
        }
      }
#ifdef GP_EMULATION
    } else if(GPIsGPImage(szFile) || cartridgeType == 2) {
      failed = !GPLoadRom(szFile);
      if(!failed) {
        cartridgeType = 2;
        emuWriteState = GPWriteState;
        emuReadState = GPReadState;
        emuWriteBattery = GPWriteBatteryFile;
        emuReadBattery = GPReadBatteryFile;
        emuReset = GPReset;
        emuCleanUp = GPCleanUp;
        emuWritePNG = GPWritePNGFile;
        emuWriteBMP = GPWriteBMPFile;
        emuMain = GPLoop;
        emuUpdateCPSR = GPUpdateCPSR;
        emuHasDebugger = true;
        emuCount = 50000;
        
        GPInit();
        GPReset();
      }
#endif
    } else {
      systemMessage(0, "Unknown file type %s", szFile);
      exit(-1);
    }
    
    if(failed) {
      systemMessage(0, "Failed to load file %s", szFile);
      exit(-1);
    }
  } else {
    cartridgeType = 0;
    strcpy(filename, "gnu_stub");
    rom = (u8 *)malloc(0x2000000);
    workRAM = (u8 *)calloc(1, 0x40000);
    bios = (u8 *)calloc(1,0x4000);
    internalRAM = (u8 *)calloc(1,0x8000);
    paletteRAM = (u8 *)calloc(1,0x400);
    vram = (u8 *)calloc(1, 0x20000);
    oam = (u8 *)calloc(1, 0x400);
    pix = (u8 *)calloc(1, 4 * 240 * 160);
    ioMem = (u8 *)calloc(1, 0x400);

    emuWriteState = CPUWriteState;
    emuReadState = CPUReadState;
    emuWriteBattery = CPUWriteBatteryFile;
    emuReadBattery = CPUReadBatteryFile;
    emuReset = CPUReset;
    emuCleanUp = CPUCleanUp;
    emuWritePNG = CPUWritePNGFile;
    emuWriteBMP = CPUWriteBMPFile;
    emuMain = CPULoop;
    emuUpdateCPSR = CPUUpdateCPSR;
    emuHasDebugger = true;
    emuCount = 50000;    
    
    CPUInit(biosFileName, useBios);
    CPUReset();    
  }
  
  sdlReadBattery();
  
  if(debuggerStub) 
    remoteInit();
  
  int flags = SDL_INIT_VIDEO|SDL_INIT_AUDIO|
    SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE;
  
  if(SDL_Init(flags)) {
    systemMessage(0, "Failed to init SDL: %s", SDL_GetError());
    exit(-1);
  }

  if(SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
    systemMessage(0, "Failed to init joystick support: %s", SDL_GetError());
  }
  
  sdlCheckKeys();
  
  if(cartridgeType == 0) {
    srcWidth = 240;
    srcHeight = 160;
    systemFrameSkip = frameSkip;
  } else if (cartridgeType == 1) {
    if(gbBorderOn) {
      srcWidth = 256;
      srcHeight = 224;
      gbBorderLineSkip = 256;
      gbBorderColumnSkip = 48;
      gbBorderRowSkip = 40;
    } else {      
      srcWidth = 160;
      srcHeight = 144;
      gbBorderLineSkip = 160;
      gbBorderColumnSkip = 0;
      gbBorderRowSkip = 0;
    }
    systemFrameSkip = gbFrameSkip;
  } else {
    srcWidth = 320;
    srcHeight = 240;
  }
  
  destWidth = (sizeOption+1)*srcWidth;
  destHeight = (sizeOption+1)*srcHeight;
  
  surface = SDL_SetVideoMode(destWidth, destHeight, 16,
                             SDL_ANYFORMAT|SDL_HWSURFACE|SDL_DOUBLEBUF|
                             (fullscreen ? SDL_FULLSCREEN : 0));
  
  if(surface == NULL) {
    systemMessage(0, "Failed to set video mode");
    SDL_Quit();
    exit(-1);
  }
  
  systemRedShift = sdlCalculateShift(surface->format->Rmask);
  systemGreenShift = sdlCalculateShift(surface->format->Gmask);
  systemBlueShift = sdlCalculateShift(surface->format->Bmask);
  
  systemColorDepth = surface->format->BitsPerPixel;
  if(systemColorDepth == 15)
    systemColorDepth = 16;

  if(yuv) {
    Init_Overlay(surface, yuvType);
    systemColorDepth = 32;
    systemRedShift = 3;
    systemGreenShift = 11;
    systemBlueShift =  19;
  }
  
  if(systemColorDepth != 16 && systemColorDepth != 24 &&
     systemColorDepth != 32) {
    fprintf(stderr,"Unsupported color depth '%d'.\nOnly 16, 24 and 32 bit color depths are supported\n", systemColorDepth);
    exit(-1);
  }

#ifndef C_CORE
  sdlMakeStretcher(srcWidth);
#else
  switch(systemColorDepth) {
  case 16:
    sdlStretcher = sdlStretcher16[sizeOption];
    break;
  case 24:
    sdlStretcher = sdlStretcher24[sizeOption];
    break;
  case 32:
    sdlStretcher = sdlStretcher32[sizeOption];
    break;
  default:
    fprintf(stderr, "Unsupported resolution: %d\n", systemColorDepth);
    exit(-1);
  }
#endif

  fprintf(stderr,"Color depth: %d\n", systemColorDepth);
  
  if(systemColorDepth == 16) {
    if(sdlCalculateMaskWidth(surface->format->Gmask) == 6) {
      Init_2xSaI(565);
      RGB_LOW_BITS_MASK = 0x821;
    } else {
      Init_2xSaI(555);
      RGB_LOW_BITS_MASK = 0x421;      
    }
    if(cartridgeType == 2) {
      for(int i = 0; i < 0x10000; i++) {
        systemColorMap16[i] = (((i >> 1) & 0x1f) << systemBlueShift) |
          (((i & 0x7c0) >> 6) << systemGreenShift) |
          (((i & 0xf800) >> 11) << systemRedShift);  
      }      
    } else {
      for(int i = 0; i < 0x10000; i++) {
        systemColorMap16[i] = ((i & 0x1f) << systemRedShift) |
          (((i & 0x3e0) >> 5) << systemGreenShift) |
          (((i & 0x7c00) >> 10) << systemBlueShift);  
      }
    }
    srcPitch = srcWidth * 2+2;
  } else {
    if(systemColorDepth != 32)
      filterFunction = NULL;
    RGB_LOW_BITS_MASK = 0x010101;
    if(systemColorDepth == 32) {
      Init_2xSaI(32);
    }
    for(int i = 0; i < 0x10000; i++) {
      systemColorMap32[i] = ((i & 0x1f) << systemRedShift) |
        (((i & 0x3e0) >> 5) << systemGreenShift) |
        (((i & 0x7c00) >> 10) << systemBlueShift);  
    }
    if(systemColorDepth == 32)
      srcPitch = srcWidth*4 + 4;
    else
      srcPitch = srcWidth*3;
  }

  if(systemColorDepth != 32) {
    switch(filter) {
    default:
    case 0:
      filterFunction = NULL;
      break;
    case 1:
      filterFunction = TVMode;
      break;
    case 2:
      filterFunction = _2xSaI;
      break;
    case 3:
      filterFunction = Super2xSaI;
      break;
    case 4:
      filterFunction = SuperEagle;
      break;
    case 5:
      filterFunction = Pixelate;
      break;
    case 6:
      filterFunction = MotionBlur;
      break;
    case 7:
      filterFunction = AdMame2x;
      break;
    case 8:
      filterFunction = Simple2x;
      break;
    case 9:
      filterFunction = Bilinear;
      break;
    case 10:
      filterFunction = BilinearPlus;
      break;
    }
  } else {
    switch(filter) {
    case 0:
      filterFunction = NULL;
      break;
    case 1:
      filterFunction = TVMode32;
      break;
    case 2:
      filterFunction = _2xSaI32;
      break;
    case 3:
      filterFunction = Super2xSaI32;
      break;
    case 4:
      filterFunction = SuperEagle32;
      break;
    case 5:
      filterFunction = Pixelate32;
      break;
    case 6:
      filterFunction = MotionBlur32;
      break;
    case 7:
      filterFunction = AdMame2x32;
      break;
    case 8:
      filterFunction = Simple2x32;
      break;
    case 9:
      filterFunction = Bilinear32;
      break;
    case 10:
      filterFunction = BilinearPlus32;
      break;
    default:
      filterFunction = NULL;
      break;
    }
  }
  
  if(systemColorDepth == 16) {
    switch(ifbType) {
    case 0:
    default:
      ifbFunction = NULL;
      break;
    case 1:
      ifbFunction = MotionBlurIB;
      break;
    case 2:
      ifbFunction = SmartIB;
      break;
    }
  } else if(systemColorDepth == 32) {
    switch(ifbType) {
    case 0:
    default:
      ifbFunction = NULL;
      break;
    case 1:
      ifbFunction = MotionBlurIB32;
      break;
    case 2:
      ifbFunction = SmartIB32;
      break;
    }
  } else
    ifbFunction = NULL;
  
  emulating = 1;
  renderedFrames = 0;

  soundInit();

  autoFrameSkipLastTime = throttleLastTime = systemGetClock();
  
  SDL_WM_SetCaption("VisualBoyAdvance", NULL);

  while(emulating) {
    if(!paused) {
      if(debugger && emuHasDebugger)
        dbgMain();
      else
        emuMain(emuCount);
    } else {
      SDL_Delay(500);
    }
    sdlPollEvents();
    if(mouseCounter) {
      mouseCounter--;
      if(mouseCounter == 0)
        SDL_ShowCursor(SDL_DISABLE);
    }
  }
  
  emulating = 0;
  fprintf(stderr,"Shutting down\n");
  remoteCleanUp();
  soundShutdown();

  if(gbRom != NULL || rom != NULL) {
    sdlWriteBattery();
    emuCleanUp();
  }

  if(delta) {
    free(delta);
    delta = NULL;
  }
  
  SDL_Quit();
  return 0;
}

void systemMessage(int num, char *msg, ...)
{
  char buffer[2048];
  va_list valist;
  
  va_start(valist, msg);
  vsprintf(buffer, msg, valist);
  
  fprintf(stderr, "%s\n", buffer);
  va_end(valist);
}

void systemDrawScreen()
{
  renderedFrames++;
  
  if(yuv) {
    Draw_Overlay(surface, sizeOption+1);
    return;
  }
  
  if(delta == NULL) {
    delta = (u8*)malloc(322*242*4);
    memset(delta, 255, 322*242*4);
  }

  SDL_LockSurface(surface);

  if(screenMessage) {
    if(cartridgeType == 1 && gbBorderOn) {
      gbSgbRenderBorder();
    }
    if(((systemGetClock() - screenMessageTime) < 3000) &&
       !disableStatusMessages) {
      int pitch = srcPitch;
      fontDisplayString(pix, srcPitch, 10, srcHeight - 20,
                        screenMessageBuffer); 
    } else {
      screenMessage = false;
    }
  }

  if(ifbFunction) {
    if(systemColorDepth == 16)
      ifbFunction(pix+destWidth+2, destWidth+2, srcWidth, srcHeight);
    else
      ifbFunction(pix+destWidth*2+4, destWidth*2+4, srcWidth, srcHeight);
  }
  
  if(filterFunction) {
    if(systemColorDepth == 16)
      filterFunction(pix+destWidth+2,destWidth+2, delta,
                     (u8*)surface->pixels,surface->pitch,
                     srcWidth,
                     srcHeight);
    else
      filterFunction(pix+destWidth*2+4,
                     destWidth*2+4,
                     delta,
                     (u8*)surface->pixels,
                     surface->pitch,
                     srcWidth,
                     srcHeight);
  } else {
    int destPitch = surface->pitch;
    u8 *src = pix;
    u8 *dest = (u8*)surface->pixels;
    int i;
    u32 *stretcher = (u32 *)sdlStretcher;
    if(systemColorDepth == 16)
      src += srcPitch;
    int option = sizeOption;
    if(yuv)
      option = 0;
    switch(sizeOption) {
    case 0:
      for(i = 0; i < srcHeight; i++) {
        SDL_CALL_STRETCHER;
        src += srcPitch;
        dest += destPitch;
      }
      break;
    case 1:
      for(i = 0; i < srcHeight; i++) {
        SDL_CALL_STRETCHER;     
        dest += destPitch;
        SDL_CALL_STRETCHER;
        src += srcPitch;
        dest += destPitch;
      }
      break;
    case 2:
      for(i = 0; i < srcHeight; i++) {
        SDL_CALL_STRETCHER;
        dest += destPitch;
        SDL_CALL_STRETCHER;
        dest += destPitch;
        SDL_CALL_STRETCHER;
        src += srcPitch;
        dest += destPitch;
      }
      break;
    case 3:
      for(i = 0; i < srcHeight; i++) {
        SDL_CALL_STRETCHER;
        dest += destPitch;
        SDL_CALL_STRETCHER;
        dest += destPitch;
        SDL_CALL_STRETCHER;
        dest += destPitch;
        SDL_CALL_STRETCHER;
        src += srcPitch;
        dest += destPitch;
      }
      break;
    }
  }

  if(showSpeed && fullscreen) {
    char buffer[50];
    if(showSpeed == 1)
      sprintf(buffer, "%d%%", systemSpeed);
    else
      sprintf(buffer, "%3d%%(%d, %d fps)", systemSpeed,
              systemFrameSkip,
              showRenderedFrames);
    if(showSpeedTransparent)
      fontDisplayStringTransp((u8*)surface->pixels,
                              surface->pitch,
                              10,
                              surface->h-20,
                              buffer);
    else
      fontDisplayString((u8*)surface->pixels,
                        surface->pitch,
                        10,
                        surface->h-20,
                        buffer);        
  }  

  SDL_UnlockSurface(surface);
  //  SDL_UpdateRect(surface, 0, 0, destWidth, destHeight);
  SDL_Flip(surface);
}

bool systemReadJoypads()
{
  return true;
}

u32 systemReadJoypad(int which)
{
  if(which < 0 || which > 3)
    which = sdlDefaultJoypad;
  
  u32 res = 0;
  
  if(sdlButtons[which][KEY_BUTTON_A])
    res |= 1;
  if(sdlButtons[which][KEY_BUTTON_B])
    res |= 2;
  if(sdlButtons[which][KEY_BUTTON_SELECT])
    res |= 4;
  if(sdlButtons[which][KEY_BUTTON_START])
    res |= 8;
  if(sdlButtons[which][KEY_RIGHT])
    res |= 16;
  if(sdlButtons[which][KEY_LEFT])
    res |= 32;
  if(sdlButtons[which][KEY_UP])
    res |= 64;
  if(sdlButtons[which][KEY_DOWN])
    res |= 128;
  if(sdlButtons[which][KEY_BUTTON_R])
    res |= 256;
  if(sdlButtons[which][KEY_BUTTON_L])
    res |= 512;

  if(sdlButtons[which][KEY_BUTTON_SPEED])
    res |= 1024;
  if(sdlButtons[which][KEY_BUTTON_CAPTURE])
    res |= 2048;

  if(autoFire) {
    res &= (~autoFire);
    if(autoFireToggle)
      res |= autoFire;
    autoFireToggle = !autoFireToggle;
  }
  
  return res;
}

void systemSetTitle(char *title)
{
  SDL_WM_SetCaption(title, NULL);
}

void systemShowSpeed(int speed)
{
  systemSpeed = speed;

  showRenderedFrames = renderedFrames;
  renderedFrames = 0;  

  if(!fullscreen && showSpeed) {
    char buffer[80];
    if(showSpeed == 1)
      sprintf(buffer, "VisualBoyAdvance-%3d%%", systemSpeed);
    else
      sprintf(buffer, "VisualBoyAdvance-%3d%%(%d, %d fps)", systemSpeed,
              systemFrameSkip,
              showRenderedFrames);

    systemSetTitle(buffer);
  }
}

void system10Frames(int rate)
{
  u32 time = systemGetClock();  
  if(!wasPaused && autoFrameSkip && !throttle) {
    u32 diff = time - autoFrameSkipLastTime;
    int speed = 100;

    if(diff)
      speed = (1000000/rate)/diff;
    
    if(speed >= 98) {
      frameskipadjust++;

      if(frameskipadjust >= 3) {
        frameskipadjust=0;
        if(systemFrameSkip > 0)
          systemFrameSkip--;
      }
    } else {
      if(speed  < 80)
        frameskipadjust -= (90 - speed)/5;
      else if(systemFrameSkip < 9)
        frameskipadjust--;

      if(frameskipadjust <= -2) {
        frameskipadjust += 2;
        if(systemFrameSkip < 9)
          systemFrameSkip++;
      }
    }    
  }
  if(!wasPaused && throttle) {
    if(!speedup) {
      u32 diff = time - throttleLastTime;
      
      int target = (1000000/(rate*throttle));
      int d = (target - diff);
      
      if(d > 0) {
        SDL_Delay(d);
      }
    }
    throttleLastTime = systemGetClock();
  }
  wasPaused = false;
  autoFrameSkipLastTime = time;
}

void systemScreenCapture(int a)
{
  char buffer[2048];

  if(captureFormat) {
    if(captureDir[0])
      sprintf(buffer, "%s/%s%02d.bmp", captureDir, sdlGetFilename(filename), a);
    else
      sprintf(buffer, "%s%02d.bmp", filename, a);

    emuWriteBMP(buffer);
  } else {
    if(captureDir[0])
      sprintf(buffer, "%s/%s%02d.png", captureDir, sdlGetFilename(filename), a);
    else
      sprintf(buffer, "%s%02d.png", filename, a);
    emuWritePNG(buffer);
  }

  systemScreenMessage("Screen capture");
}

void soundCallback(void *,u8 *stream,int len)
{
  if(!emulating)
    return;
  SDL_mutexP(mutex);
  //  printf("Locked mutex\n");
  if(!speedup && !throttle) {
    while(sdlSoundLen < 2048*2) {
      if(emulating)
        SDL_CondWait(cond, mutex);
      else 
        break;
    }
  }
  if(emulating) {
    //  printf("Copying data\n");
    memcpy(stream, sdlBuffer, len);
  }
  sdlSoundLen = 0;
  if(mutex)
    SDL_mutexV(mutex);
}

void systemWriteDataToSoundBuffer()
{
  if(SDL_GetAudioStatus() != SDL_AUDIO_PLAYING)
    SDL_PauseAudio(0);
  bool cont = true;
  while(cont && !speedup && !throttle) {
    SDL_mutexP(mutex);
    //    printf("Waiting for len < 2048 (speed up %d)\n", speedup);
    if(sdlSoundLen < 2048*2)
      cont = false;
    SDL_mutexV(mutex);
  }

  int len = soundBufferLen;
  int copied = 0;
  if((sdlSoundLen+len) >= 2048*2) {
    //    printf("Case 1\n");
    memcpy(&sdlBuffer[sdlSoundLen],soundFinalWave, 2048*2-sdlSoundLen);
    copied = 2048*2 - sdlSoundLen;
    sdlSoundLen = 2048*2;
    SDL_CondSignal(cond);
    cont = true;
    if(!speedup && !throttle) {
      while(cont) {
        SDL_mutexP(mutex);
        if(sdlSoundLen < 2048*2)
          cont = false;
        SDL_mutexV(mutex);
      }
      memcpy(&sdlBuffer[0],&(((u8 *)soundFinalWave)[copied]),
             soundBufferLen-copied);
      sdlSoundLen = soundBufferLen-copied;
    } else {
      memcpy(&sdlBuffer[0], &(((u8 *)soundFinalWave)[copied]), 
soundBufferLen);
    }
  } else {
    //    printf("case 2\n");
    memcpy(&sdlBuffer[sdlSoundLen], soundFinalWave, soundBufferLen);
    sdlSoundLen += soundBufferLen;
  }
}

bool systemSoundInit()
{
  SDL_AudioSpec audio;

  switch(soundQuality) {
  case 1:
    audio.freq = 44100;
    soundBufferLen = 1470*2;
    break;
  case 2:
    audio.freq = 22050;
    soundBufferLen = 736*2;
    break;
  case 4:
    audio.freq = 11025;
    soundBufferLen = 368*2;
    break;
  }
  audio.format=AUDIO_S16SYS;
  audio.channels = 2;
  audio.samples = 1024;
  audio.callback = soundCallback;
  audio.userdata = NULL;
  if(SDL_OpenAudio(&audio, NULL)) {
    fprintf(stderr,"Failed to open audio: %s\n", SDL_GetError());
    return false;
  }
  soundBufferTotalLen = soundBufferLen*10;
  cond = SDL_CreateCond();
  mutex = SDL_CreateMutex();
  sdlSoundLen = 0;
  systemSoundOn = true;
  return true;
}

void systemSoundShutdown()
{
  SDL_mutexP(mutex);
  SDL_CondSignal(cond);
  SDL_mutexV(mutex);
  SDL_DestroyCond(cond);
  cond = NULL;
  SDL_DestroyMutex(mutex);
  mutex = NULL;
  SDL_CloseAudio();
}

void systemSoundPause()
{
  SDL_PauseAudio(1);
}

void systemSoundResume()
{
  SDL_PauseAudio(0);
}

void systemSoundReset()
{
}

u32 systemGetClock()
{
  return SDL_GetTicks();
}

void systemUpdateMotionSensor()
{
  if(sdlMotionButtons[KEY_LEFT]) {
    sensorX += 3;
    if(sensorX > 2197)
      sensorX = 2197;
    if(sensorX < 2047)
      sensorX = 2057;
  } else if(sdlMotionButtons[KEY_RIGHT]) {
    sensorX -= 3;
    if(sensorX < 1897)
      sensorX = 1897;
    if(sensorX > 2047)
      sensorX = 2037;
  } else if(sensorX > 2047) {
    sensorX -= 2;
    if(sensorX < 2047)
      sensorX = 2047;
  } else {
    sensorX += 2;
    if(sensorX > 2047)
      sensorX = 2047;
  }

  if(sdlMotionButtons[KEY_UP]) {
    sensorY += 3;
    if(sensorY > 2197)
      sensorY = 2197;
    if(sensorY < 2047)
      sensorY = 2057;
  } else if(sdlMotionButtons[KEY_DOWN]) {
    sensorY -= 3;
    if(sensorY < 1897)
      sensorY = 1897;
    if(sensorY > 2047)
      sensorY = 2037;
  } else if(sensorY > 2047) {
    sensorY -= 2;
    if(sensorY < 2047)
      sensorY = 2047;
  } else {
    sensorY += 2;
    if(sensorY > 2047)
      sensorY = 2047;
  }    
}

int systemGetSensorX()
{
  return sensorX;
}

int systemGetSensorY()
{
  return sensorY;
}

void systemGbPrint(u8 *data,int pages,int feed,int palette, int contrast)
{
}

void systemScreenMessage(char *msg)
{
  screenMessage = true;
  screenMessageTime = systemGetClock();
  if(strlen(msg) > 20) {
    strncpy(screenMessageBuffer, msg, 20);
    screenMessageBuffer[20] = 0;
  } else
    strcpy(screenMessageBuffer, msg);  
}

bool systemCanChangeSoundQuality()
{
  return false;
}

bool systemPauseOnFrame()
{
  if(pauseNextFrame) {
    paused = true;
    pauseNextFrame = false;
    return true;
  }
  return false;
}

// Code donated by Niels Wagenaar (BoycottAdvance)

// GBA screensize.
#define GBA_WIDTH   240
#define GBA_HEIGHT  160

void Init_Overlay(SDL_Surface *gbascreen, int overlaytype)
{
  
  overlay = SDL_CreateYUVOverlay( GBA_WIDTH,
                                  GBA_HEIGHT,
                                  overlaytype, gbascreen);
  fprintf(stderr, "Created %dx%dx%d %s %s overlay\n",
          overlay->w,overlay->h,overlay->planes,
          overlay->hw_overlay?"hardware":"software",
          overlay->format==SDL_YV12_OVERLAY?"YV12":
          overlay->format==SDL_IYUV_OVERLAY?"IYUV":
          overlay->format==SDL_YUY2_OVERLAY?"YUY2":
          overlay->format==SDL_UYVY_OVERLAY?"UYVY":
          overlay->format==SDL_YVYU_OVERLAY?"YVYU":
          "Unknown");
}

void Quit_Overlay(void)
{
  
  SDL_FreeYUVOverlay(overlay);
}

/* NOTE: These RGB conversion functions are not intended for speed,
   only as examples.
*/
inline void RGBtoYUV(Uint8 *rgb, int *yuv)
{
  yuv[0] = (int)((0.257 * rgb[0]) + (0.504 * rgb[1]) + (0.098 * rgb[2]) + 16);
  yuv[1] = (int)(128 - (0.148 * rgb[0]) - (0.291 * rgb[1]) + (0.439 * rgb[2]));
  yuv[2] = (int)(128 + (0.439 * rgb[0]) - (0.368 * rgb[1]) - (0.071 * rgb[2]));
}

inline void ConvertRGBtoYV12(SDL_Overlay *o)
{
  int x,y;
  int yuv[3];
  Uint8 *p,*op[3];
  
  SDL_LockYUVOverlay(o);
  
  /* Black initialization */
  /*
    memset(o->pixels[0],0,o->pitches[0]*o->h);
    memset(o->pixels[1],128,o->pitches[1]*((o->h+1)/2));
    memset(o->pixels[2],128,o->pitches[2]*((o->h+1)/2));
  */
  
  /* Convert */
  for(y=0; y<160 && y<o->h; y++) {
    p=(Uint8 *)pix+srcPitch*y;
    op[0]=o->pixels[0]+o->pitches[0]*y;
    op[1]=o->pixels[1]+o->pitches[1]*(y/2);
    op[2]=o->pixels[2]+o->pitches[2]*(y/2);
    for(x=0; x<240 && x<o->w; x++) {
      RGBtoYUV(p,yuv);
      *(op[0]++)=yuv[0];
      if(x%2==0 && y%2==0) {
        *(op[1]++)=yuv[2];
        *(op[2]++)=yuv[1];
      }
      p+=4;//s->format->BytesPerPixel;
    }
  }
  
  SDL_UnlockYUVOverlay(o);
}

inline void ConvertRGBtoIYUV(SDL_Overlay *o)
{
  int x,y;
  int yuv[3];
  Uint8 *p,*op[3];
  
  SDL_LockYUVOverlay(o);
  
  /* Black initialization */
  /*
    memset(o->pixels[0],0,o->pitches[0]*o->h);
    memset(o->pixels[1],128,o->pitches[1]*((o->h+1)/2));
    memset(o->pixels[2],128,o->pitches[2]*((o->h+1)/2));
  */
  
  /* Convert */
  for(y=0; y<160 && y<o->h; y++) {
    p=(Uint8 *)pix+srcPitch*y;
    op[0]=o->pixels[0]+o->pitches[0]*y;
    op[1]=o->pixels[1]+o->pitches[1]*(y/2);
    op[2]=o->pixels[2]+o->pitches[2]*(y/2);
    for(x=0; x<240 && x<o->w; x++) {
      RGBtoYUV(p,yuv);
      *(op[0]++)=yuv[0];
      if(x%2==0 && y%2==0) {
        *(op[1]++)=yuv[1];
        *(op[2]++)=yuv[2];
      }
      p+=4; //s->format->BytesPerPixel;
    }
  }
  
  SDL_UnlockYUVOverlay(o);
}

inline void ConvertRGBtoUYVY(SDL_Overlay *o)
{
  int x,y;
  int yuv[3];
  Uint8 *p,*op;
  
  SDL_LockYUVOverlay(o);
  
  for(y=0; y<160 && y<o->h; y++) {
    p=(Uint8 *)pix+srcPitch*y;
    op=o->pixels[0]+o->pitches[0]*y;
    for(x=0; x<240 && x<o->w; x++) {
      RGBtoYUV(p,yuv);
      if(x%2==0) {
        *(op++)=yuv[1];
        *(op++)=yuv[0];
        *(op++)=yuv[2];
      } else
        *(op++)=yuv[0];
      
      p+=4; //s->format->BytesPerPixel;
    }
  }
  
  SDL_UnlockYUVOverlay(o);
}

inline void ConvertRGBtoYVYU(SDL_Overlay *o)
{
  int x,y;
  int yuv[3];
  Uint8 *p,*op;
  
  SDL_LockYUVOverlay(o);
  
  for(y=0; y<160 && y<o->h; y++) {
    p=(Uint8 *)pix+srcPitch*y;
    op=o->pixels[0]+o->pitches[0]*y;
    for(x=0; x<240 && x<o->w; x++) {
      RGBtoYUV(p,yuv);
      if(x%2==0) {
        *(op++)=yuv[0];
        *(op++)=yuv[2];
        op[1]=yuv[1];
      } else {
        *op=yuv[0];
        op+=2;
      }
      
      p+=4; //s->format->BytesPerPixel;
    }
  }
  
  SDL_UnlockYUVOverlay(o);
}

inline void ConvertRGBtoYUY2(SDL_Overlay *o)
{
  int x,y;
  int yuv[3];
  Uint8 *p,*op;
  
  SDL_LockYUVOverlay(o);
  
  for(y=0; y<160 && y<o->h; y++) {
    p=(Uint8 *)pix+srcPitch*y;
    op=o->pixels[0]+o->pitches[0]*y;
    for(x=0; x<240 && x<o->w; x++) {
      RGBtoYUV(p,yuv);
      if(x%2==0) {
        *(op++)=yuv[0];
        *(op++)=yuv[1];
        op[1]=yuv[2];
      } else {
        *op=yuv[0];
        op+=2;
      }
      
      p+=4; //s->format->BytesPerPixel;
    }
  }
  
  SDL_UnlockYUVOverlay(o);
}

inline void Convert32bit(SDL_Surface *display)
{
  switch(overlay->format) {
  case SDL_YV12_OVERLAY:
    ConvertRGBtoYV12(overlay);
    break;
  case SDL_UYVY_OVERLAY:
    ConvertRGBtoUYVY(overlay);
    break;
  case SDL_YVYU_OVERLAY:
    ConvertRGBtoYVYU(overlay);
    break;
  case SDL_YUY2_OVERLAY:
    ConvertRGBtoYUY2(overlay);
    break;
  case SDL_IYUV_OVERLAY:
    ConvertRGBtoIYUV(overlay);
    break;
  default:
    fprintf(stderr, "cannot convert RGB picture to obtained YUV format!\n");
    exit(1);
    break;
  }
  
}


inline void Draw_Overlay(SDL_Surface *display, int size)
{
  SDL_LockYUVOverlay(overlay);
  
  Convert32bit(display);
  
  overlay_rect.x = 0;
  overlay_rect.y = 0;
  overlay_rect.w = GBA_WIDTH  * size;
  overlay_rect.h = GBA_HEIGHT * size;

  SDL_DisplayYUVOverlay(overlay, &overlay_rect);
  SDL_UnlockYUVOverlay(overlay);
}
