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

#import "SpriteController.h"
#include "GBA.h"
#include "Port.h"
#include "Globals.h"

typedef struct {
  union {
    u16 attr0;
    struct {
#ifdef __BIG_ENDIAN__
      u16 shape : 2;
      u16 eightbit : 1;
      u16 mosaic : 1;
      u16 mode : 2;
      u16 doubled : 1;
      u16 transformed : 1;
      u16 y : 8;
#else
      u16 y : 8;
      u16 transformed : 1;
      u16 doubled : 1;
      u16 mode : 2;
      u16 mosaic : 1;
      u16 eightbit : 1;
      u16 shape : 2;
#endif
    };
  };
  union {
    u16 attr1;
    struct {
#ifdef __BIG_ENDIAN__
      u16 size : 2;
      u16 transformation : 5;
      u16 x : 9;
#else
      u16 x : 9;
      u16 transformation : 5;
      u16 size : 2;
#endif
    };
  };
  union {
    u16 attr2;
    struct {
#ifdef __BIG_ENDIAN__
      u16 palette : 4;
      u16 priority : 2;
      u16 tile : 10;
#else
      u16 tile : 10;
      u16 priority : 2;
      u16 palette : 4;
#endif
    };
  };
} OAMEntry;

static NSError* _error = nil;

@interface VBOAMEntry : NSObject {
  u16 tile;
  u16 attr0;
  u16 attr1;
  u16 attr2;
  u16 index;
  u16 x;
  u8 shape;
  u8 eightbit;
  u8 mosaic;
  u8 mode;
  u8 doubled;
  u8 transformed;
  u8 y;
  u8 size;
  u8 transformation;
  u8 palette;
  u8 priority;
}
-(id)initWithOAMEntry:(OAMEntry*)e index:(u16)i;
@end

#define k_OAMEntrySize 8
static void readOAMEntry(u8 num, OAMEntry *entry) {
  u32 addr = 0x7000000 + k_OAMEntrySize*num;
  entry->attr0 = debuggerReadHalfWord(addr);
  entry->attr1 = debuggerReadHalfWord(addr + 2);
  entry->attr2 = debuggerReadHalfWord(addr + 4);
}

static void decodeSizes(OAMEntry *e, char *w, char *h) {
  char *small, *big;
  *w = *h = 8 << e->size;
  if (e->shape == 1) {
    small = h;
    big = w;
  } else if (e->shape == 2) {
    small = w;
    big = h;
  } else {
    return;
  }
  switch (e->size) {
  case 0:
    *big *= 2;
    break;
  case 1:
    *big *= 2;
    // fall through
  case 2:
  case 3:
    *small /= 2;
    break;
  }
}

@implementation VBOAMEntry
-(id)initWithOAMEntry:(OAMEntry*)e index:(u16)i{
  self = [super init];
  index = i;
  attr0 = e->attr0;
  attr1 = e->attr1;
  attr2 = e->attr2;
  shape = e->shape;
  eightbit = e->eightbit;
  mosaic = e->mosaic;
  mode = e->mode;
  doubled = e->doubled;
  y = e->y;
  size = e->size;
  transformation = e->transformation;
  x = e->x;
  palette = e->palette;
  priority = e->priority;
  tile = e->tile;
  return self;
}
-(NSString*)attr0 {
  return [NSString stringWithFormat:@"%4.4x", attr0];
}
-(NSString*)attr1 {
  return [NSString stringWithFormat:@"%4.4x", attr1];
}
-(NSString*)attr2 {
  return [NSString stringWithFormat:@"%4.4x", attr2];
}
-(NSError*)modificationError {
  if (_error == nil) {
    NSDictionary *userInfoDict =
      [NSDictionary dictionaryWithObject:@"Memory modification not implemented"
                                  forKey:NSLocalizedDescriptionKey];
    _error = [[NSError alloc] initWithDomain:@"VBAstep"
                                        code:1
                                    userInfo:userInfoDict];
  }
  return _error;
}

- (BOOL)validateValue:(id *)ioValue forKey:(NSString *)key error:(NSError **)outError {
  *outError = [self modificationError];
  return NO;
}
- (BOOL)isDisabled {
  return !transformed && doubled;
}
- (BOOL)xflipped {
  return (BOOL)(transformation & 4);
}
- (BOOL)yflipped {
  return (BOOL)(transformation & 8);
}


@end

@implementation SpriteController

- (void)awakeFromNib {
  [grid setTiles: 128];
  [grid setCols: 16];
  [grid setAction:@selector(selectTile:)];
  [grid setOpaque: NO];
  [grid setSelection: 0];

  [tile setSelectionColor: [NSColor blueColor]];
}

- (void)dealloc {
  [_error dealloc];
  [super dealloc];
}

- (void) tileGrid:(TileGrid*)_grid willDrawTile:(int)which selected:(BOOL)b {
  unsigned char *data = vram + 0x10000;
  OAMEntry e;
  char w, h;
  
  // don't crash if emu isn't started!
  if (!vram)
    return;

  readOAMEntry((u8)(which & 127), &e);
  decodeSizes(&e, &w, &h);
  
  if (e.eightbit)
    pals[which] = 0xFFFF;
  else
    pals[which] = 1 << e.palette;

  //if ((DISPCNT & 7) > 2)
  //  data += 0x4000;

  [tile setSelected:b];
  [tile set8Bit: e.eightbit];
  [tile setPalBase: 256 + (e.eightbit ? 0 : 16* e.palette)];
  [tile setTileData: data + 32*e.tile];
  [tile setHeight: h];
  [tile setWidth: w];
}

- (void)displayEntry:(unsigned)which {
  OAMEntry e;
  if (!oam)
    return;
  readOAMEntry(which, &e);
  [self setValue: [[[VBOAMEntry alloc] initWithOAMEntry:&e index:which]
                    autorelease]
        forKey: @"selection"];  
}

- (IBAction)selectTile:(id)sender {
  u8 which = [sender getSelection] & 127;
  [self displayEntry: which];
}

- (void) entryChanged:(unsigned)which {
  if (which == [grid getSelection])
    [self displayEntry: which];
  [grid setTileNeedsDisplay: which];
};

-(void)updateEntries:(VBBitMap*)entries palettes:(u16)palettes
{
  unsigned oamIndex;
  [entries performSelectorWithSelectedBits:@selector(entryChanged:)
           onObject: self];
  if (palettes == 0)
    return;
  for (oamIndex = 0; oamIndex < 128; oamIndex++) {
    if (pals[oamIndex] & palettes)
      [grid setTileNeedsDisplay: oamIndex];
  }
}
@end
