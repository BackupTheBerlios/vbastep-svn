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

#import "ColorTile.h"

@implementation ColorTile

- (void)setColor:(NSColor*)aColor {
  color = aColor;
}

- (void)setBGColor:(NSColor*)aColor {
  background = aColor;
}

- (NSSize)size {
  return size;
}

- (void)setSize:(NSSize)theSize {
  size = theSize;
}

- (void)setWidth:(int)w {
  size.width = w;
}

- (void)setHeight:(int)h {
  size.height = h;
}

- (void)setSelected:(BOOL)b {
  selected = b;
}

- (void)drawAtPoint:(NSPoint)p inView:(NSView*)view {
  NSRect r;
  r.origin = p;
  r.size = size;
  [color set];
  NSRectFill(NSInsetRect(r, 1, 1));
  if (selected) {
    [background set];
    NSFrameRectWithWidth(r,2.0f);
  }   
}

@end
