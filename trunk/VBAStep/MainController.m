// VBAStep - Nintendo Gameboy Advance Debugger
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

#import "MainController.h"
#import "Emulator.h"
#import "GbaScreen.h"

#include "GBA.h"

void initSystem();
static MainController *controllerInstance = nil;


@implementation MainController

- (void)awakeFromNib {
  autoscroll = YES;
}

- (id) init {
  emulator = nil;
  screenImage = nil;
  autoscroll = YES;
  return self;
}

- (void)applicationWillFinishLaunching: (NSNotification *)aNotification
{
  controllerInstance = self;
  initSystem();
}

- (void)loadRom:(NSString*)filename
{
  [emulator shutDown];
  if (emulator == nil)
    emulator = [[Emulator alloc] init];
  [[NSDocumentController sharedDocumentController]
    noteNewRecentDocumentURL: [NSURL fileURLWithPath: filename]];
  [emulator loadRom:filename];
  screenImage = [emulator getImageRep];
  [display setImageRep: screenImage];
}

- (BOOL)application:(NSApplication *)theApp openFile:(NSString*)filename
{
  [self loadRom:filename];
  [gbaWindow makeKeyAndOrderFront:self];
  [emulator startRunning];
  return YES;
}

+ (void)refreshDisplays
{
  [controllerInstance performSelectorOnMainThread:@selector(updateDisplay:)
                      withObject:nil
                      waitUntilDone:NO];
}

- (void)updateDisplay:(id)sender {
  [display setNeedsDisplay:YES];
  [memLogger updateWindows];
}

- (void)openDocument:(id)sender {
  NSOpenPanel *panel = [NSOpenPanel openPanel];
  if ([panel runModalForTypes:[NSArray arrayWithObjects:@"gba",nil]]
      == NSOKButton) {
    NSArray *filenames = [panel filenames];
    NSString *rom = [filenames objectAtIndex:0];
    [self application:nil
          openFile:rom];
  }
}

- (void)updateTitle:(int)speed
{
  [gbaWindow setTitle:[NSString stringWithFormat:@"VBAstep (%d%%)",
                                systemSpeed]];
}

- (void)displayPaused:(NSString*)type {
  [gbaWindow setTitle:[NSString stringWithFormat:@"VBAstep (%@)", type]];
}

+ (void)displaySpeed:(int)speed
{
  // Could have this call setTitle on the window,
  // but have to make sure string is correctly retained
  controllerInstance->systemSpeed = speed;
  [controllerInstance performSelectorOnMainThread:@selector(updateTitle:)
                      withObject:nil//(id)speed
                      waitUntilDone:NO];
}


-(void) toggleLayer:(id)sender
{
  unsigned int mask = 0x0100 << [sender tag];
  if ([sender state]) {
    layerSettings &= ~mask;
  } else {
    layerSettings |= mask;
  }
  layerEnable = DISPCNT & layerSettings;
}

- (void)windowWillClose:(NSNotification*)notification
{
  if ([notification object] == gbaWindow) {
    [emulator shutDown];
  }
}

- (void)showLayers:(id)sender
{
  [layersWindow orderFront:self];
}

- (void) applicationWillTerminate:(NSNotification*)not
{
  [emulator shutDown];
}

- (void)logMessage:(NSString *)msg
{
  NSRange endRange;

  endRange.location = [[logView textStorage] length];
  endRange.length = 0;
  [logView replaceCharactersInRange:endRange withString:msg];
  if (autoscroll) {
    endRange.length = [msg length];
    [logView scrollRangeToVisible: endRange];
  }
}

- (IBAction)clearLog:(id)sender {
  NSAttributedString *empty = [[NSAttributedString alloc] initWithString:@""];
  [[logView textStorage] setAttributedString: empty];
  [empty release];
}

+ (void)appendToLog:(NSString *)str
{
  [controllerInstance performSelectorOnMainThread:@selector(logMessage:)
                      withObject:[str retain]
                      waitUntilDone:NO];
}

- (void)showLog:(id)sender
{
  [logWindow orderFront:self];
}

- (Emulator*)emulator {
  return emulator;
}

- (void) startEmu {
    [gbaWindow makeKeyAndOrderFront:self];
    [emulator startRunning];
}
#if 0
+ (void) delayForMillis:(int)millis {
  NSTimeInterval seconds = (NSTimeInterval)millis/1000.0;
  [NSThread sleepUntilDate: [NSDate dateWithTimeIntervalSinceNow: seconds]];
}
#endif
@end

