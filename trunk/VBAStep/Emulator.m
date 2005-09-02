// VBAStep - Nintendo Gameboy Advance Debugger
// Copyright (C) 2005 Rib Rdb

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

#import "Emulator.h"
#import "MainController.h"
#include <string.h>
#include "GBA.h"

#define COND_BLOCK_EMU 1
#define COND_UNBLOCK_EMU 2
#define COND_EMU_SHUTDOWN 3

extern int emulating;

void debuggerMain()
{
}

void debuggerSignal(int a, int b)
{
}

void debuggerOutput(char *a, u32 b)
{
  NSString *str;
  BOOL frees = NO;
  if (!a) {
    int size = 256;
    char *buf;
    char c = debuggerReadByte(b);
    b += 1;
    a = malloc(size);
    buf = a;
    while (c) {
      if ( buf - a >= size - 1 ) {
        int ofs = buf - a;
        size *= 2;
        a = realloc(a, size);
        buf = a + ofs;
      }
      *(buf++) = c;
      c = debuggerReadByte(b);
      b += 1;
    }
    *buf = 0;
    frees = YES;
  }
  str = [[NSString alloc] initWithCString:a];
  [MainController appendToLog:str];
  [str release];
  if (frees)
    free(a);
}

void debuggerBreakOnWrite(u32 address, u32 oldvalue, u32 value, 
                          int size, int t)
{
}
 
void (*dbgMain)() = debuggerMain;
void (*dbgSignal)(int, int) = debuggerSignal;
void (*dbgOutput)(char *, u32) = debuggerOutput;

extern void remoteInit();
extern void remoteCleanUp();
extern void remoteStubMain();
extern void remoteStubSignal(int,int);
extern void remoteOutput(char *, u32);
extern void remoteSetProtocol(int);
extern void remoteSetPort(int);

extern char cpuBreakLoop;

@implementation Emulator

- (id) init
{
  self = [super init];
  gba = GBASystem;
  shutdown = YES;
  emulating = NO;
  lock = [[NSConditionLock alloc] init];
  return self;
}

- (void) dealloc
{
  [lock release];
  [super dealloc];
}

- (NSImageRep*) getImageRep
{
  NSBitmapImageRep *rep = [NSBitmapImageRep alloc];
  unsigned char *planes[2] = {((unsigned char*)pix) + 4*241, nil };
  //memset(pix, 0xFFFFFFFF, 4*241*162);
  rep = [rep initWithBitmapDataPlanes:planes
             pixelsWide:240
             pixelsHigh:160
             bitsPerSample:8
             samplesPerPixel:3
             hasAlpha:NO
             isPlanar:NO
             colorSpaceName:NSDeviceRGBColorSpace
             bytesPerRow:4*241
             bitsPerPixel:32 ];
  return [rep autorelease];
}

- (BOOL)loadRom:(NSString *)filename
{
  int size = CPULoadRom([filename cString]);
  if ( size <= 0 ) {
    return NO;
  }
  CPUInit("", NO);
  CPUReset();

  // XXX: load battery
  if (dbgMain == remoteStubMain) {
    remoteCleanUp();
    [self setRemoteDebugger:NO];
  }
  debugger = NO;
  return YES;
}
extern void remoteStubMain();

- (void)run:(id)sender
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  // Cocoa shouldn't be called from this thread so it
  // should be fine to release this at the end
  while (emulating) {
    if (NO) { // XXX: paused
      [lock lockWhenCondition: COND_UNBLOCK_EMU];
      [lock unlock];
    } else {
      if (debugger)
        remoteStubMain();
      else
        gba.emuMain(gba.emuCount);
    }
  }
  [lock lock];
  shutdown = YES;
  [lock unlockWithCondition: COND_EMU_SHUTDOWN];
  [pool release];
}

- (void)startRunning {
  shutdown = NO;
  emulating = YES;
  [NSThread detachNewThreadSelector:@selector(run:)
            toTarget:self
            withObject:self];
}

- (void)shutDown {
  if (shutdown)
    return;
  // XXX: Make sure nothing else can touch the condition!!
  emulating = NO;
  cpuBreakLoop = YES;
  [lock lockWhenCondition: COND_EMU_SHUTDOWN];
  [lock unlock];
}

- (void)setRemoteDebugger:(BOOL)hasDebugger {
  if (hasDebugger) {
    dbgMain = remoteStubMain;
    dbgSignal = remoteStubSignal;
    dbgOutput = remoteOutput;
  } else {
    dbgMain = debuggerMain;
    dbgSignal = debuggerSignal;
    dbgOutput = debuggerOutput;
  }
}

@end
