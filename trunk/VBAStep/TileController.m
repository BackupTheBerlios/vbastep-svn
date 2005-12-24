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

#import "TileController.h"
#include "GBA.h"

@implementation TileController

- (void)awakeFromNib {
  NSBitmapImageRep *rep;
  image = [[NSImage alloc] initWithSize: NSMakeSize(8,8)];
  [image setCacheMode: NSImageCacheNever];
  [image setFlipped: YES];
#if 0 /* Let NSImage allocate it's own reg */
  rep = [[NSBitmapImageRep alloc]
          initWithBitmapDataPlanes: nil
          pixelsWide: 8
          pixelsHigh: 8
          bitsPerSample: 8
          samplesPerPixel: 3
          hasAlpha: NO
          isPlanar: NO
          colorSpaceName: NSDeviceRGBColorSpace
          bytesPerRow: 8*4
          bitsPerPixel: 32];
  [image addRepresentation: rep];
  [rep release];
#endif
  [grid setTiles: 1024];
  [grid setCols: 32];
  [grid setAction:@selector(selectTile:)];
  [grid setOpaque: YES];

  [tile setSelectionColor: [NSColor blueColor]];
  [preview setImage: image];

  charData = 0x6000000;
}

- (IBAction)depthChanged:(id)sender
{
  eightbit = [[sender selectedCell] tag];
  [grid setNeedsDisplay:YES];
  if (eightbit) {
    [self setPalBase: palBase & 256];
  }
  [paletteStepper setEnabled: !eightbit];
}

- (IBAction)tabSelected:(id)sender
{
  int selection = [[sender selectedCell] tag];
  int newbase;
  charData = 0x6000000 + 32*1024*selection;
  if (selection == 2) // sprites
    newbase = (palBase % 256) + 256;
  else
    newbase = palBase % 256;
  [self setPalBase: newbase];
  [grid setNeedsDisplay:YES];
}

- (unsigned int)addressForTile:(int)which {
  return charData + 32*(eightbit+1)*which;
}

- (void)updatePreview {
  int selection = [grid getSelection];
  if (selection >= 0) {
    [image lockFocus];
    [self tileGrid: nil
          willDrawTile: selection
          selected: NO];
    [tile drawAtPoint: NSZeroPoint inView: nil];
    [image unlockFocus];
  }
  [preview setImage: nil];
  [preview setImage: image];
}

- (IBAction)selectTile:(id)sender
{
  [self updatePreview];
  [addressField setStringValue:
                  [NSString stringWithFormat:@"0x%x",
                            [self addressForTile: [sender getSelection]]]];
}

- (void) tileGrid:(TileGrid*)_grid willDrawTile:(int)which selected:(BOOL)b {
  unsigned int address = [self addressForTile: which];
  unsigned char *data = vram + (address & 0x1FFFF);
  // don't crash if emu isn't started!
  if (!vram)
    return;
  [tile setSelected:b];
  [tile set8Bit: eightbit];
  [tile setPalBase: palBase];
  [tile setTileData: data];
}

- (IBAction)paletteChanged:(id)sender
{
  int newbase = (palBase & 256) | (16*[sender intValue]);
  [self setPalBase: newbase];
}

- (void)setPalBase:(int)base
{
  palBase = base;
  base = ((base / 16) % 16);
  [paletteField setIntValue:base];
  [paletteStepper setIntValue:base];
  [self updatePreview];
  [grid setNeedsDisplay: YES];
}
@end
