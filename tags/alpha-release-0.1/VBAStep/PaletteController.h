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
#import "ColorTile.h"

@interface PaletteController : NSObject
{
    IBOutlet id address;
    IBOutlet TileGrid *bgGrid;
    IBOutlet NSColorWell *colorWell;
    IBOutlet id html;
    IBOutlet TileGrid *spriteGrid;
    IBOutlet ColorTile *tile;
    IBOutlet id value;
}
- (IBAction)colorSelected:(id)sender;
- (IBAction)editTile:(id)sender;
- (IBAction)selectTile:(id)sender;
@end
