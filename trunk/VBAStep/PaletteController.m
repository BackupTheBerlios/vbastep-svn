#import "PaletteController.h"

@implementation PaletteController

- (void)awakeFromNib {
  [bgGrid setTiles:256];
  [bgGrid setCols:16];
  [spriteGrid setTiles:256];
  [spriteGrid setCols:16];
  [tile setBGColor:[NSColor blueColor]];
  [tile setColor:[NSColor blueColor]];
  [tile setSize:NSMakeSize(12,8)];
  [spriteGrid setNeedsDisplay:YES];
  [bgGrid setNeedsDisplay:YES];
  //[[grid window] sizeToFitContent];
}
- (IBAction)colorSelected:(id)sender
{
}

- (IBAction)editTile:(id)sender
{
}
- (void) tileGrid:(TileGrid*)_grid willDrawTile:(int)which selected:(BOOL)b {
  [tile setSelected:b];
}
- (IBAction)selectTile:(id)sender
{
}

@end
