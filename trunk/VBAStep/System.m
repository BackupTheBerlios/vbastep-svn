// VBAStep - Copyright (C) 2005 Rib Rdb
// Based on System.cpp from the GTK version of VisualBoyAdvance
// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2004 Forgotten and the VBA development team

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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "GBA.h"
#include "Util.h"
#include "System.h"

#import "MainController.h"
#import <Foundation/Foundation.h>

NSDate *startTime = nil;
u32 gsKeyState;

// Required vars, used by the emulator core
//
int  systemRedShift;
int  systemGreenShift;
int  systemBlueShift;
int  systemColorDepth;
int  systemDebug;
int  systemVerbose;
int  systemSaveUpdateCounter;
int  systemFrameSkip;
u32  systemColorMap32[0x10000];
u16  systemColorMap16[0x10000];
u16  systemGbPalette[24];
char systemSoundOn;

int  emulating;
char debugger;
int  RGB_LOW_BITS_MASK;


// Extra vars, only used for the GUI
//
int systemRenderedFrames;
int systemFPS;


void systemMessage(int _iId, const char * msg, ...)
{
  va_list args;
  va_start(args, msg);

  vfprintf(stderr, msg, args);

  va_end(args);
}

void systemDrawScreen()
{
  [MainController refreshDisplays];
  systemRenderedFrames++;
}

char systemReadJoypads()
{
  return YES;
}

u32 systemReadJoypad(int ignored)
{
  return gsKeyState;
}

void systemShowSpeed(int _iSpeed)
{
  systemFPS = systemRenderedFrames;
  systemRenderedFrames = 0;

  [MainController displaySpeed:_iSpeed];
}

void system10Frames(int _iRate)
{
  // XXX: GUI()->vComputeFrameskip(_iRate);
}

void systemFrame()
{
}

void systemSetTitle(const char * _csTitle)
{
  //XXX: GUI()->set_title(_csTitle);
}

void systemScreenCapture(int _iNum)
{
  //XXX: GUI()->vCaptureScreen(_iNum);
}

void systemWriteDataToSoundBuffer()
{
}

char systemSoundInit()
{
  return NO;
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

u32 systemGetClock()
{
  return (u32)(-1000*[startTime timeIntervalSinceNow]);
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

void systemGbPrint(u8 * _puiData,
                   int  _iPages,
                   int  _iFeed,
                   int  _iPalette,
                   int  _iContrast)
{
}

void systemScreenMessage(const char * _csMsg)
{
}

char systemCanChangeSoundQuality()
{
  return YES;
}

char systemPauseOnFrame()
{
  return NO;
}

void systemGbBorderOn()
{
}


void initSystem() {
  int i;
  if (startTime != nil)
    [startTime release];
  startTime = [[NSDate alloc] init];
#ifdef __BIG_ENDIAN__
  systemRedShift    = 27;
  systemGreenShift  = 19;
  systemBlueShift   = 11;
  RGB_LOW_BITS_MASK = 0x01010100;
#else
  systemRedShift    = 3;
  systemGreenShift  = 11;
  systemBlueShift   = 19;
  RGB_LOW_BITS_MASK = 0x00010101;
#endif

  systemColorDepth = 32;
  systemDebug = 0;
  systemVerbose = 0;
  systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;
  systemFrameSkip = 0;
  systemSoundOn = NO;
  //soundOffFlag = YES;

  systemRenderedFrames = 0;
  systemFPS = 0;

  emulating = 0;
  debugger = YES;

  for (i = 0; i < 0x10000; i++)
  {
    systemColorMap32[i] = (((i & 0x1f) << systemRedShift)
                           | (((i & 0x3e0) >> 5) << systemGreenShift)
                           | (((i & 0x7c00) >> 10) << systemBlueShift));
  }
  //remoteSetProtocol(1);
  //remoteInit();
}
