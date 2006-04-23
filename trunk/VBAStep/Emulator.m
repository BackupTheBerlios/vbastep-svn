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
#include <sys/types.h>
#include <sys/socket.h>
#include "GBA.h"

#define COND_EMU_RUNNING 1
#define COND_EMU_SHUTTING_DOWN 2
#define COND_EMU_SHUTDOWN 3

#define COND_UNPAUSED 1
#define COND_PAUSING 2
#define COND_PAUSED 3

extern int remoteInit();
extern BOOL remoteTcpConnect(int);
extern void remoteCleanUp();
extern void remoteStubMain();
extern void remoteStubSignal(int,int);
extern void remoteOutput(char *, u32);
extern void remoteSetProtocol(int);
extern void remoteSetPort(int);

void debuggerMain();
void debuggerSignal(int, int);
void debuggerOutput(char*, u32);

extern char cpuBreakLoop;
extern int emulating;
extern BOOL wasPaused;


void (*dbgMain)() = debuggerMain;
void (*dbgSignal)(int, int) = debuggerSignal;
void (*dbgOutput)(char *, u32) = debuggerOutput;

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
  if (dbgMain == remoteStubMain) {
    remoteOutput(a, 0);
  }

  if (frees)
    free(a);
}

@implementation Emulator

- (id) init
{
  self = [super init];
  gba = GBASystem;
  emulating = NO;
  pauseLock = [[NSConditionLock alloc] initWithCondition: COND_UNPAUSED];
  shutdownLock = [[NSConditionLock alloc] initWithCondition: COND_EMU_SHUTDOWN];
  return self;
}

- (void) dealloc
{
  [pauseLock release];
  [shutdownLock release];
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
extern void remoteStubCheckForBreak();
- (void)run:(id)sender
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  // Cocoa shouldn't be called from this thread so it
  // should be fine to release this at the end

  [shutdownLock lockWhenCondition: COND_EMU_SHUTDOWN];
  [shutdownLock unlockWithCondition: COND_EMU_RUNNING];
  [pauseLock lock];
  // make sure no one is left waiting for a resume
  [pauseLock unlockWithCondition: COND_PAUSED];
  [pauseLock lock];
  [pauseLock unlockWithCondition: COND_UNPAUSED];

  emulating = YES;

  while (emulating) {
    if ([pauseLock tryLockWhenCondition: COND_PAUSING]) {
      [pauseLock unlockWithCondition: COND_PAUSED];
      [pauseLock lockWhenCondition: COND_UNPAUSED];
      [pauseLock unlock];
    } else {
      if (debugger) {
        remoteStubMain();
      } else {
        if (dbgMain == remoteStubMain)
          remoteStubCheckForBreak();
        gba.emuMain(gba.emuCount);
      }
    }
  }
  [shutdownLock lock];
  [shutdownLock unlockWithCondition: COND_EMU_SHUTDOWN];
  [pool release];
}

- (void)startRunning {
  [NSThread detachNewThreadSelector:@selector(run:)
            toTarget:self
            withObject:self];
}

- (void)shutDown {
  BOOL first = NO;
  [shutdownLock lock];
  switch ([shutdownLock condition]) {
  case COND_EMU_SHUTDOWN:
    break;

  case COND_EMU_RUNNING:
    first = YES;
    emulating = NO;
    cpuBreakLoop = YES;

    [self resume];

    // Fall through

  case COND_EMU_SHUTTING_DOWN:
    [shutdownLock unlockWithCondition: COND_EMU_SHUTTING_DOWN];
    [shutdownLock lockWhenCondition: COND_EMU_SHUTDOWN];
  }
  if (first) {
    if (dbgMain == remoteStubMain) {
      remoteCleanUp();
      [self setRemoteDebugger:NO];
    }
    debugger = NO;
  }

  [shutdownLock unlockWithCondition: COND_EMU_SHUTDOWN];
}

- (void)reset {
  if ([self emulating]) {
    gba.emuReset();
  }
}

- (void)pause {
  if (![self emulating])
    return;
  [pauseLock lock];
  switch ([pauseLock condition]) {
  case COND_PAUSED:
    break;
  case COND_UNPAUSED:
    wasPaused = YES;
    cpuBreakLoop = YES;
    // fall through
  case COND_PAUSING:
    [pauseLock unlockWithCondition: COND_PAUSING];
    [pauseLock lockWhenCondition: COND_PAUSED];
  }
  [pauseLock unlockWithCondition: COND_PAUSED];
}

- (void)resume {
  [pauseLock lock];
  switch ([pauseLock condition]) {
  case COND_UNPAUSED:
    break;
  case COND_PAUSING:
    [pauseLock unlock];
    [pauseLock lockWhenCondition: COND_PAUSED];
  case COND_PAUSED:
    break;
  }
  [pauseLock unlockWithCondition: COND_UNPAUSED];
}

- (void)setRemoteDebugger:(BOOL)hasDebugger {
  if (hasDebugger) {
    dbgMain = remoteStubMain;
    dbgSignal = remoteStubSignal;
    dbgOutput = debuggerOutput;
  } else {
    dbgMain = debuggerMain;
    dbgSignal = debuggerSignal;
    dbgOutput = debuggerOutput;
  }
}

- (int)remoteInit:(int)port {
  remoteSetProtocol(0);
  remoteSetPort(port);
  return remoteInit();
}

- (void)remoteConnect:(id)sender {
  [self pause];
  [NSThread detachNewThreadSelector:@selector(_waitForGdb:)
            toTarget:self
            withObject:sender];
}

- (void)cancelRemoteConnect {
  char c = 1;
  if (cancelSock)
    (void)write(cancelSock, &c, 1);
}

- (void)_waitForGdb:(id)sender {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  BOOL result = NO;
  int sockets[2];

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0) {
    cancelSock = sockets[0];
    result = remoteTcpConnect(sockets[1]);
    close(sockets[0]);
    close(sockets[1]);
    cancelSock = 0;
  }
  if ([sender respondsToSelector:@selector(remoteConnectSucceeded:)]) {
    [sender remoteConnectSucceeded:result];
  }
  [self setRemoteDebugger:result];
  [pool release];
}

- (BOOL)running {
  return ([pauseLock condition] == COND_UNPAUSED) && [self emulating];
}

- (BOOL)emulating {
  return emulating && ([shutdownLock condition] == COND_EMU_RUNNING);
}

@end
