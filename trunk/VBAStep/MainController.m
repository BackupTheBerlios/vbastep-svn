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
#import <Renaissance/Renaissance.h>

#include "GBA.h"

void initSystem();
static MainController *controllerInstance = nil;
static Emulator *emulator = nil;
static NSImageRep *screenImage = nil;
static int systemSpeed;

@implementation MainController

- (void)applicationWillFinishLaunching: (NSNotification *)aNotification
{
  controllerInstance = self;
  [NSBundle loadGSMarkupNamed: @"MainWindow"  owner: self];
  initSystem();
}

- (void)loadRom:(NSString*)filename
{
  [emulator shutDown];
  if (emulator == nil)
    emulator = [[Emulator alloc] init];
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
  [gbaWindow setTitle:[NSString stringWithFormat:@"VBA Step (%d%%)",
                                systemSpeed]];
}

+ (void)displaySpeed:(int)speed
{
  // Could have this call setTitle on the window,
  // but have to make sure string is correctly retained
  systemSpeed = speed;
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

- (void)connectToGdb:(id)sender
{
  NSOpenPanel *panel = [NSOpenPanel openPanel];
  [gbaWindow orderOut:self];
  [panel setAccessoryView:selectPortView];
  if ([panel runModalForTypes:[NSArray arrayWithObjects:@"gba",nil]]
      == NSOKButton) {
    int port = [selectPortField intValue];
    NSArray *filenames = [panel filenames];
    NSString *rom = [filenames objectAtIndex:0];

    [self loadRom:rom];

    remoteSetProtocol(0);
    remoteSetPort(port);
    remoteInit();

    debugger = YES;

    [emulator setRemoteDebugger: YES];
    [gbaWindow makeKeyAndOrderFront:self];
    [emulator startRunning];
  }
}

- (void) applicationWillTerminate:(NSNotification*)not
{
  [emulator shutDown];
}

- (void)logMessage:(NSString *)msg
{
  NSAttributedString *amsg = [[NSAttributedString alloc] initWithString:msg];
  [[logView textStorage] appendAttributedString: amsg ];
  [amsg release];
  [msg release];
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

@end

