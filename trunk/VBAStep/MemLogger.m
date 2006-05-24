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
#import "System.h"

static MemLogger *instance = nil;
static u32 gbaRead(void *_data, u32 offset, u32 length) {
  u8 *data = _data;
  u32 result = 0;
  u8 i;
  for (i = 0; i < length; i++) {
    result |= data[offset + i] << 8*i;
  }
  return result;
}


@implementation MemLogger
- (id)init {
  self = [super init];
  instance = self;
  colorLog = [[BitMapPair alloc] initWithLength: 512];
  oamLog = [[BitMapPair alloc] initWithLength: 128];
  palLog = [[BitMapPair alloc] initWithLength: 32];
  return self;
}

- (void) dealloc {
  [colorLog release];
  [oamLog release];
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
  u16 modifiedSpritePalettes;
  if (trackPals) {
    [colorLog performSelectorWithSelectedBits:@selector(colorChanged:)
              onObject: palController];
  }
  modifiedSpritePalettes = EndianU16_LtoN(((u16*)[palLog data])[1]);
  [spriteController updateEntries: [oamLog readMap]
                           palettes: modifiedSpritePalettes ];
  [colorLog swapBitMaps];
  [palLog swapBitMaps];
  [oamLog swapBitMaps];
  if (trackTiles && tileLog)
    [tiles setNeedsDisplay: YES];
  tileLog = NO;
}

@end

void registerModified(u32 address) {

}

void paletteRamModified(u32 address, u32 length, u32 newval) {
  (void)newval;
  address /= 2; // pal entries are 16 bits
  [instance->colorLog setBit: address];
  if (length > 2)
      [instance->colorLog setBit: address + 1];
  [instance->palLog setBit: address / 16]; // 16 colors / pal
}

void vramModified(u32 address) {
  // TODO real tile tracking
  instance->tileLog = YES;
  //  if (address >= 0x6000000)
  //    instance->spriteLog = YES;
}

extern u8* oam;
void oamModified(u32 address, u32 length, u32 new) {
  u32 old = gbaRead(oam, address, length);
  if (old == new)
    return;
  address /= 8;
  [instance->oamLog setBit: address];
}
