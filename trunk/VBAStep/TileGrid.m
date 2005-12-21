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

#import "TileGrid.h"

@implementation TileGrid

// For Renaissance

- (NSSize)minimumSizeForContent {
  NSSize tileSize = [standardTile size];
  return NSMakeSize(tileSize.width*cols,
                    tileSize.height*[self rows]);
}

- (void) sizeToFitContent {
  [self setFrameSize: [self minimumSizeForContent]];
}

// Getter/Setters
- (id) initWithFrame:(NSRect)frame {
  self = [super initWithFrame:frame];
  selection = -1;
  return self;
}

- (id) init {
  return [self initWithFrame:NSZeroRect];
}

- (id)delegate {
  return delegate;
}

- (void)setDelegate:(id)_delegate {
  delegate = _delegate;
}

- (id)target {
  return target;
}

- (void)setTarget:(id)_target {
  target = _target;
}

- (SEL)action {
  return action;

}

- (void)setAction:(SEL)aSel {
  action = aSel;
}

- (SEL)doubleAction {
  return doubleAction;
}

- (void)setDoubleAction:(SEL)aSel {
  doubleAction = aSel;
}

- (id<Tile>)tile {
  return standardTile;
}

- (void)setTile:(id<Tile>)aTile {
  standardTile = aTile;
}

- (id<Tile>)selectedTile {
  return selectedTile;
}

- (void)setSelectedTile:(id<Tile>)aTile {
  selectedTile = aTile;
}

- (int)rows {
  return (tiles + cols - 1)/cols;
}

- (int)tiles {
  return tiles;
}

- (void)setTiles:(int)_tiles {
  tiles = _tiles;
}

- (int)cols {
  return cols;
}

- (void)setCols:(int)_cols {
  cols = _cols;
}

- (int)tileAtPoint:(NSPoint)point {
  NSRect r;
  r.size = [self minimumSizeForContent];
  r.origin = NSZeroPoint;
  if ([self mouse:point inRect:r]) {
    NSSize tileSize = [standardTile size];
    int row = point.y/tileSize.height;
    int col = point.x/tileSize.width;
    return row*cols + col;
  } else {
    return -1;
  }
}

- (int)getSelection {
  return selection;
}

- (void)setSelection:(int)tileno {
  if (selection == tileno)
    return;

  [self setTileNeedsDisplay:selection];
  [self setTileNeedsDisplay:tileno];

  selection = tileno;
  
  if ([target respondsToSelector:action])
    [target performSelector:action withObject:self];
}

// For NSView
- (BOOL)isFlipped {
  return YES;
}

// XXX: This could be optimized more
- (void)drawRect:(NSRect)aRect {
  NSRect r = NSZeroRect;
  int width = [self minimumSizeForContent].width;
  int curtile;
  id<Tile> tile;

  r.size = [standardTile size];

  for (curtile = 0; curtile < tiles; curtile++) {
    if ([self needsToDrawRect: r]) {
      // notify delegate (who should prepare the tile to draw)
      if ([delegate respondsToSelector:
                      @selector(tileGrid:willDrawTile:selected:)]) {
        [delegate tileGrid:self
                  willDrawTile:curtile
                  selected:curtile == selection];
      }
      // choose which tile to draw
      if (curtile == selection && selectedTile != nil) {
        tile = selectedTile;
      } else {
        tile = standardTile;
      }
      // and draw it
      [tile drawAtPoint:r.origin inView:self];
    }
    // figure out where to draw the next tile
    r.origin.x += r.size.width;
    if (r.origin.x >= width) {
      r.origin.x = 0;
      r.origin.y += r.size.height;
    }
  }
}

- (NSRect) rectForTile:(int)which {
  NSRect r;
  r.size = [standardTile size];
  r.origin.x = r.size.width*(which % cols);
  r.origin.y = r.size.height*(which / cols);
  return r;
}

- (NSToolTipTag)addToolTipForTile:(int)tileno 
                            owner:(id)owner
                         userData:(void*)data {
  return [self addToolTipRect: [self rectForTile: tileno]
               owner: owner
               userData: data];
}

- (int) tileForEvent:(NSEvent *)theEvent {
  return [self tileAtPoint:[self convertPoint:
                                   [theEvent locationInWindow]
                                 fromView: nil]];
}

- (NSMenu *)menuForEvent:(NSEvent *)theEvent {
  if ([delegate respondsToSelector: @selector(tileGrid:menuForTile:event:)]) {
    int tileno = [self tileForEvent: theEvent];
    return [delegate tileGrid: self
                     menuForTile: tileno
                     event: theEvent];
  } else {
    return [super menuForEvent:theEvent];
  }
}

- (void)mouseDown:(NSEvent*)theEvent {
  int tileno = [self tileForEvent: theEvent];
  if (tileno == -1)
    return;
  if (selection != tileno) {
    [self setSelection: tileno];
  }
  if ([theEvent clickCount] == 2
      && [target respondsToSelector:doubleAction]) {
    [target performSelector:doubleAction withObject: self];
  }
}

- (void)setTileNeedsDisplay:(int)tileno {
  [self setNeedsDisplayInRect: [self rectForTile: tileno]];
}

- (void)setOpaque:(BOOL)b {
  opaque = b;
}

- (BOOL)isOpaque {
  return opaque;
}
@end
