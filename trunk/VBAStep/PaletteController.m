#import "PaletteController.h"
#include "GBA.h"
#include "Globals.h"
#include "Port.h"

NSColor *colorForGbaColor(unsigned int colorval) {
  float r, g, b;
  colorval = systemColorMap32[colorval];
  r = ((colorval >> 24) & 0xff)/255.0f;
  g = ((colorval >> 16) & 0xff)/255.0f;
  b = ((colorval >>  8) & 0xff)/255.0f;
  return [NSColor colorWithDeviceRed:r green:g blue:b alpha:1.0];
}
inline unsigned int gbaColorForId(int id) {
  if (paletteRAM)
    return debuggerReadHalfWord(0x5000000 + 2*id);
  else
    return 0;
}

@implementation PaletteController

- (void)awakeFromNib {
  TileGrid *grids[2] = {bgGrid, spriteGrid};
  int i;
  [tile setBGColor:[NSColor blueColor]];
  [tile setColor:[NSColor blueColor]];
  [tile setSize:NSMakeSize(12,8)];
  for (i = 0; i < 2; i++) {
    [grids[i] setTiles:256];
    [grids[i] setCols:16];
    [grids[i] setAction:@selector(selectTile:)];
    [grids[i] setDoubleAction:@selector(editTile:)];
    [grids[i] setNeedsDisplay:YES];
  }
}
- (IBAction)colorSelected:(id)sender
{
  NSColor *color = [sender color];
  int r, g, b;
  int selection;
  r = (int)31*[color redComponent];
  g = (int)31*[color greenComponent];
  b = (int)31*[color blueComponent];
  selection = [bgGrid getSelection];
  if (selection == -1) {
    selection = [spriteGrid getSelection];
    if (selection == -1)
      return;
    selection += 256;
  }
  debuggerWriteHalfWord((0x5000000 + 2*selection), (b<<10)|(g<<5)|(r));
}

- (IBAction)editTile:(id)sender
{
  [colorWell activate:YES];
}
- (void) tileGrid:(TileGrid*)_grid willDrawTile:(int)which selected:(BOOL)b {
  [tile setSelected:b];
  if (_grid == spriteGrid)
    which += 256;
  [tile setColor:colorForGbaColor(gbaColorForId(which))];
}
- (IBAction)selectTile:(id)sender
{
  int colorid = [sender getSelection];
  unsigned int colorval;
  if (colorid < 0)
    return;

  if (sender == spriteGrid) {
    colorid += 256;
    [bgGrid setSelection: -1];
  } else {
    [spriteGrid setSelection: -1];
  }
  [address setStringValue:
             [NSString stringWithFormat:@"0x05000%3.3x",2*colorid]];
  colorval = gbaColorForId(colorid);
  [value setStringValue:
           [NSString stringWithFormat:@"0x%4.4x",colorval]];
  [html setStringValue:
          [NSString stringWithFormat:@"#%6.6x",
                    systemColorMap32[colorval] >> 8 ]];

  [colorWell setColor:colorForGbaColor(colorval)];
}
-(void)colorChanged:(unsigned)which {
  if (which < 256)
    [bgGrid setTileNeedsDisplay: which];
  else
    [spriteGrid setTileNeedsDisplay: which & 255];
}
@end
