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

#import "SpriteTile.h"

NSRect ScaleAndCenterRect(NSSize size, NSRect bounds) {
  double xscale, yscale, scale;
  NSRect result;
  xscale = (bounds.size.width) / (size.width);
  yscale = (bounds.size.height) / (size.height);
  scale = (xscale < yscale) ? xscale : yscale;

  result.size.width = size.width * scale;
  result.size.height = size.height * scale;
  result.origin.x = bounds.origin.x +
    (bounds.size.width - result.size.width)/2.0;
  result.origin.y = bounds.origin.y +
    (bounds.size.height - result.size.height)/2.0;
  return result;
}

@implementation SpriteTile

- (id)init {
  self = [super init];
  image = [[NSImage alloc] initWithSize: NSMakeSize(64, 64)];
  return self;
}

- (void)dealloc {
  [image release];
  [super dealloc];
}

- (void)setHeight:(char) pixels {
  height = pixels / 8;
}

- (void)setWidth:(char) pixels {
  width = pixels / 8;
}

- (NSImage*)image {
  return image;
}

- (NSSize)size {
  return NSMakeSize(17,17);
}

-(void)prepareImage {
  NSPoint tp;
  unsigned char *oldData = data;
  int tileOfs = (eightbit) ? 64 : 32;
  char x, y;
  BOOL wasSelected = selected;

  selected = NO;
  [image lockFocus];

  for (y = 0; y < height; y++) {
    tp.y = 8*y;
    for (x = 0; x < width; x++) {
      tp.x = 8*x;

      [super drawAtPoint: tp inView:nil];

      data += tileOfs;
    }
  }

  [image unlockFocus];
  data = oldData;
  selected = wasSelected;
}

- (void)drawAtPoint:(NSPoint)p inView:(NSView*)view {
  NSRect src, dest;
  [self prepareImage];
  src = NSMakeRect(0, 0, width*8, height*8);
  dest = ScaleAndCenterRect(NSMakeSize(width*8, height*8),
                            NSMakeRect(p.x, p.y, 16, 16));
  [image drawInRect:dest fromRect:src operation: NSCompositeCopy fraction: 1.0];
  if (selected) {
    [selection_color set];
    NSFrameRectWithWidth(NSMakeRect(p.x,p.y,16,16),1.0f);
  }
}
@end
