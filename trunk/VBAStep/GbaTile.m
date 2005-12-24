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

#import "GbaTile.h"
#include "GBA.h"
#include "Port.h"
#include "System.h"

static inline unsigned int gbaColorForId(int id) {
  if (paletteRAM)
    return debuggerReadHalfWord(0x5000000 + 2*id);
  else
    return 0;
}

@implementation GbaTile

- (id)init {
  self = [super init];
  rep = [NSBitmapImageRep alloc];
  rep = [rep initWithBitmapDataPlanes: nil
             pixelsWide: 8
             pixelsHigh: 8
             bitsPerSample: 8
             samplesPerPixel: 3
             hasAlpha: NO
             isPlanar: NO
             colorSpaceName: NSDeviceRGBColorSpace
             bytesPerRow: 4*8
             bitsPerPixel: 32];
  return self;
}

- (void)setSelectionColor:(NSColor*)aColor {
  selection_color = aColor;
}

- (void)setSelected:(BOOL)b {
  selected = b;
}

- (void)setPalBase:(int)b {
  pal_base = b;
}

- (int)palBase {
  return pal_base;
}

- (void)setTileData:(u8*)d {
  data = d;
}

- (void)set8Bit:(BOOL)b {
  eightbit = b;
}

- (NSSize)size {
  return NSMakeSize(8,8);
}

// XXX: Invert the y axis
- (void)drawAtPoint:(NSPoint)p inView:(NSView*)view {
  u32 *repData = (u32*)[rep bitmapData];
  u8 *cur = data;
  int row, col;
  if (!data)
    return;
  if (eightbit) {
    for (row = 0; row < 8; row++) {
      for (col = 0; col < 8; col++) {
        u16 color = gbaColorForId(*cur + pal_base);
        *(repData + 8*(7 - row ) + col) = systemColorMap32[color];
        cur++;
      }
    }
  } else {
    for (row = 0; row < 8; row++) {
      for (col = 0; col < 4; col++) {
        u16 color = gbaColorForId((*cur & 0xF) + pal_base);
        *(repData + 8*(7 - row ) + 2*col) = systemColorMap32[color];
        color = gbaColorForId((*cur >> 4) + pal_base);
        *(repData + 8*(7 - row ) + 2*col + 1) = systemColorMap32[color];
        cur++;
      }
    }
  }

  [rep drawAtPoint: p];

  if (selected) {
    NSRect r = NSMakeRect(p.x, p.y, 8, 8);
    [selection_color set];
    NSFrameRectWithWidth(r,2.0f);
  }   
}

@end
