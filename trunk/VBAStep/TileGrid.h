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

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import "Tile.h"

@interface TileGrid : NSView {
  IBOutlet id<Tile> standardTile;
  IBOutlet id<Tile> selectedTile;
  IBOutlet id delegate;
  IBOutlet id target;
  SEL action;
  SEL doubleAction;
  int tiles;
  int cols;
  int selection;
}

- (id)delegate;
- (void)setDelegate:(id)_delegate;
- (id)target;
- (void)setTarget:(id)_target;
- (SEL)action;
- (void)setAction:(SEL)aSel;
- (SEL)doubleAction;
- (void)setDoubleAction:(SEL)aSel;
- (id<Tile>)tile;
- (void)setTile:(id<Tile>)aTile;
- (id<Tile>)selectedTile;
- (void)setSelectedTile:(id<Tile>)aTile;
- (int)rows;
- (int)tiles;
- (void)setTiles:(int)_tiles;
- (int)cols;
- (void)setCols:(int)_cols;
- (int)tileAtPoint:(NSPoint)point;
- (int)selection;
- (void)setSelection:(int)tileno;
- (NSToolTipTag)addToolTipForTile:(int)tileno owner:(id)owner userData:(void*)data;
- (void)setTileNeedsDisplay:(int)tileno;
@end

@interface NSObject ( TileGridDelegate )
- (void) tileGrid:(TileGrid*)grid willDrawTile:(int)tileno selected:(BOOL)s;
- (NSMenu*) tileGrid:(TileGrid*)grid menuForTile:(int)tileno event:(NSEvent*)e;
@end

