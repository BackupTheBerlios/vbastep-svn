// VBAStep - Nintendo Gameboy Advance Debugger -*- objc -*-
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



#import "System.h"
#import "Foundation/Foundation.h"
#import "AppKit/AppKit.h"

@interface Emulator: NSObject
{
  @public
  NSConditionLock *pauseLock;
  NSConditionLock *shutdownLock;
  struct EmulatedSystem gba;
  int cancelSock;
}

- (BOOL)loadRom:(NSString *)romfile;
- (NSImageRep *)getImageRep;
- (void)startRunning;
- (void)shutDown ;
- (void)setRemoteDebugger:(BOOL)hasDebugger;
- (void)reset;
- (void)pause;
- (void)resume;
- (int)remoteInit:(int)port;
- (void)remoteConnect:(id)sender;
- (void)cancelRemoteConnect;
- (BOOL)emulating;
- (BOOL)running; // emulating and not paused
@end

extern char debugger;

@interface NSObject ( RemoteConnectListener )
- (void)remoteConnectSucceeded:(BOOL)b;
@end
