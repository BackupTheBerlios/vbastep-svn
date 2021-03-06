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

#include "GBA.h"
#include "Font.h"
#include "debugger.h"
#include "Sound.h"
#include "unzip.h"
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

#ifdef MMX
extern "C" bool cpu_mmx;
#endif
extern bool soundEcho;
extern bool soundLowPass;
extern bool soundReverse;

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

int systemRedShift = 0;
int systemBlueShift = 16;
int systemGreenShift = 8;
int systemColorDepth = 32;
int systemDebug = 0;
int systemVerbose = 0;

int cartridgeType = 3;
int captureFormat = 0;

int emulating = 0;
int RGB_LOW_BITS_MASK=0x821;
u32 systemColorMap32[0x10000];
u16 systemColorMap16[0x10000];
u16 systemGbPalette[24];
char filename[2048];
char biosFileName[2048];
char captureDir[2048];
char saveDir[2048];
char batteryDir[2048];

bool paused = false;
bool debugger = true;
bool debuggerStub = false;
bool systemSoundOn = false;
bool removeIntros = false;

extern void debuggerSignal(int,int);

void (*dbgMain)() = debuggerMain;
void (*dbgSignal)(int,int) = debuggerSignal;
void (*dbgOutput)(char *, u32) = debuggerOutput;

extern bool CPUIsGBAImage(char *);
extern bool gbIsGameboyRom(char *);

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

void usage(char *cmd)
{
  printf("%s file-name\n",cmd);
}

int main(int argc, char **argv)
{
  fprintf(stderr,"VisualBoyAdvance-Test version %s\n", VERSION);
#ifdef __GNUC__
  fprintf(stderr,"Linux version\n");
#else
  fprintf(stderr,"Windows version\n");
#endif

  captureDir[0] = 0;
  saveDir[0] = 0;
  batteryDir[0] = 0;
  
  char buffer[1024];

  int op = -1;

  frameSkip = 2;
  gbBorderOn = 0;

  parseDebug = true;

  if(!debuggerStub) {
    if(argc <= 1) {
      systemMessage(0,"Missing image name");
      usage(argv[0]);
      exit(-1);
    }
  }

  for(int i = 0; i < 24;) {
    systemGbPalette[i++] = (0x1f) | (0x1f << 5) | (0x1f << 10);
    systemGbPalette[i++] = (0x15) | (0x15 << 5) | (0x15 << 10);
    systemGbPalette[i++] = (0x0c) | (0x0c << 5) | (0x0c << 10);
    systemGbPalette[i++] = 0;
  }

  if(argc == 2) {
    char *szFile = argv[optind];
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
    } else if(CPUIsGBAImage(szFile) || cartridgeType == 0) {
      failed = !CPULoadRom(szFile);
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

      CPUInit(biosFileName, useBios);
      CPUReset();
#ifdef GP_EMULATION
    } else if(GPIsGPImage(szFile) || cartridgeType == 2) {
      failed = !GPLoadRom(szFile);
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
#endif
    } else {
      systemMessage(0, "Unknown file type %s", szFile);
      exit(-1);
    }
    
    if(failed) {
      systemMessage(0, "Failed to load file %s", szFile);
      exit(-1);
    }
    strcpy(filename, szFile);
    char *p = strrchr(filename, '.');
    
    if(p)
      *p = 0;
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
  
  if(debuggerStub) 
    remoteInit();
  
  if(cartridgeType == 0) {
  } else if (cartridgeType == 1) {
    if(gbBorderOn) {
      gbBorderLineSkip = 256;
      gbBorderColumnSkip = 48;
      gbBorderRowSkip = 40;
    } else {      
      gbBorderLineSkip = 160;
      gbBorderColumnSkip = 0;
      gbBorderRowSkip = 0;
    }      
  } else {
  }

  for(int i = 0; i < 0x10000; i++) {
    systemColorMap32[i] = ((i & 0x1f) << systemRedShift) |
      (((i & 0x3e0) >> 5) << systemGreenShift) |
      (((i & 0x7c00) >> 10) << systemBlueShift);  
  }

  emulating = 1;
  soundInit();
  
  while(emulating) {
    if(!paused) {
      if(debugger && emuHasDebugger)
        dbgMain();
      else
        emuMain(emuCount);
    }
  }
  emulating = 0;
  fprintf(stderr,"Shutting down\n");
  remoteCleanUp();
  soundShutdown();

  if(gbRom != NULL || rom != NULL) {
    emuCleanUp();
  }

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
}

u32 systemReadJoypad()
{
  return 0;
}

void systemSetTitle(char *title)
{
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

u32 systemReadJoypadExtended()
{
  return 0;
}

void systemWriteDataToSoundBuffer()
{
}

bool systemSoundInit()
{
  return true;
}

void systemSoundShutdown()
{
}

void systemSoundPause()
{
}

void systemSoundResume()
{
}

void systemSoundReset()
{
}

static int ticks = 0;

u32 systemGetClock()
{
  return ticks++;
}

void systemUpdateMotionSensor()
{
}

int systemGetSensorX()
{
  return 0;
}

int systemGetSensorY()
{
  return 0;
}

void systemGbPrint(u8 *data,int pages,int feed,int palette, int contrast)
{
}

void systemScreenMessage(char *msg)
{
}

bool systemCanChangeSoundQuality()
{
  return false;
}

bool systemPauseOnFrame()
{
  return false;
}
