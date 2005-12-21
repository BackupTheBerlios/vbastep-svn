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

#import "MemLogger.h"
#include "System.h"

static MemLogger *instance = nil;

@implementation MemLogger
- (id)init {
  self = [super init];
  instance = self;
  palLog = [[PCList alloc] init];
  return self;
}

- (void) dealloc {
  [palLog release];
  [super dealloc];
}

- (void)windowDidBecomeKey:(NSNotification*)notification
{
  id window = [notification object];
  if (window == palWindow) {
    trackPals = YES;
  } else if (window == tileWindow) {
    trackTiles = YES;
  }
}

- (void)windowWillClose:(NSNotification*)notification
{
  id window = [notification object];
  if (window == palWindow) {
    trackPals = NO;
  } else if (window == tileWindow) {
    trackTiles = NO;
  }
}

- (void)updateWindows
{
  int i = [palLog size];
  if (trackPals) {
    while (i-- > 0) {
      int j;
      u32 color = [palLog itemAtIndex:i]/2;
      for (j = 0; j < 2; j++) {
        if (color + j< 256) {
          [bgPal setTileNeedsDisplay: color + j];
        } else {
          [spritePal setTileNeedsDisplay: ((color + j) & 255)];
        }
      }
    }
  }
  [palLog swap];
  if (trackTiles && tileLog)
    [tiles setNeedsDisplay: YES];
  tileLog = NO;
}

@end

void registerModified(u32 address) {

}

void paletteRamModified(u32 address) {
  [instance->palLog addItem: address & 0xfffffffc];
}

void vramModified(u32 address) {
  // TODO real tile tracking
  instance->tileLog = YES;
}

void oamModified(u32 address) {

}
