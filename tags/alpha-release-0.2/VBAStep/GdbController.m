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

#import "GdbController.h"

@implementation GdbController

- (IBAction)cancelConnection:(id)sender
{
  canceled = YES;
  [[mainController emulator] cancelRemoteConnect];
}

- (IBAction)okPressed:(id)sender
{
  int port;
  Emulator *emu = [mainController emulator];

  if ([emu emulating]) {
    port = [portField intValue];
  } else {
    port = [portField2 intValue];
  }

  [connectDialog orderOut:self];

  port = [emu remoteInit:port];
  [portLabel setIntValue:port];
  [progressBar startAnimation:self];
  [waitDialog makeKeyAndOrderFront:self];
  [emu remoteConnect:self];
}

- (IBAction)showConnectDialog:(id)sender
{
  Emulator *emu = [mainController emulator];
  if ([emu emulating]) {
    [emu pause];
    [connectDialog orderFront:self];
  } else {
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    [connectDialog orderOut:self];
    [panel setAccessoryView: portView];
    if ([panel runModalForTypes:[NSArray arrayWithObjects:@"gba",nil]]
        == NSOKButton) {
      NSString *rom = [[panel filenames] objectAtIndex:0];
      [mainController loadRom:rom];

      [self okPressed:self];
    }
  }
}

- (void)remoteConnectSucceeded:(BOOL)b {
  [waitDialog orderOut:self];
  [progressBar stopAnimation:self];
  if (b) {
    Emulator *emu = [mainController emulator];
    debugger = YES;
    if ([emu emulating]) {
      [emu resume];
    } else {
      [mainController startEmu];
    }
  } else if (!canceled) {
    NSRunAlertPanel(@"Error",@"Error connecting to gdb",nil, nil, nil);
  }
}

@end
