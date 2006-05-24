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

#import "Keys.h"

@implementation MainController (Keys)

-(void)keyDown:(NSEvent *)event
{
  if (![event isARepeat]) {
    NSString *chars = [event characters];
    if ([chars length] == 1) {
      switch ([chars characterAtIndex:0]) {
      case L_KEY:
        gsKeyState |= L_KEY_FLAG;
        break;
      case R_KEY:
        gsKeyState |= R_KEY_FLAG;
        break;
      case A_KEY:
        gsKeyState |= A_KEY_FLAG;
        break;
      case B_KEY:
        gsKeyState |= B_KEY_FLAG;
        break;
      case START_KEY:
        gsKeyState |= START_KEY_FLAG;
        break;
      case SELECT_KEY:
        gsKeyState |= SELECT_KEY_FLAG;
        break;
      case UP_KEY:
        gsKeyState |= UP_KEY_FLAG;
        break;
      case DOWN_KEY:
        gsKeyState |= DOWN_KEY_FLAG;
        break;
      case LEFT_KEY:
        gsKeyState |= LEFT_KEY_FLAG;
        break;
      case RIGHT_KEY:
        gsKeyState |= RIGHT_KEY_FLAG;
        break;        
      }
    }
  }
}

-(void)keyUp:(NSEvent *)event
{
  if (![event isARepeat]) {
    NSString *chars = [event characters];
    if ([chars length] == 1) {
      switch ([chars characterAtIndex:0]) {
      case L_KEY:
        gsKeyState &= ~L_KEY_FLAG;
        break;
      case R_KEY:
        gsKeyState &= ~R_KEY_FLAG;
        break;
      case A_KEY:
        gsKeyState &= ~A_KEY_FLAG;
        break;
      case B_KEY:
        gsKeyState &= ~B_KEY_FLAG;
        break;
      case START_KEY:
        gsKeyState &= ~START_KEY_FLAG;
        break;
      case SELECT_KEY:
        gsKeyState &= ~SELECT_KEY_FLAG;
        break;
      case UP_KEY:
        gsKeyState &= ~UP_KEY_FLAG;
        break;
      case DOWN_KEY:
        gsKeyState &= ~DOWN_KEY_FLAG;
        break;
      case LEFT_KEY:
        gsKeyState &= ~LEFT_KEY_FLAG;
        break;
      case RIGHT_KEY:
        gsKeyState &= ~RIGHT_KEY_FLAG;
        break;
    /* Emu Controls */
      case PAUSE_KEY:
        if ([emulator running]) {
          [emulator pause];
          [self displayPaused: @"Paused"];
        } else if ([emulator emulating]) {
          [emulator resume];
        }
        break;
      case RESET_KEY:
        [emulator reset];
        break;
      case CLOSE_KEY:
        [gbaWindow orderOut: self];
        break;
      case BREAK_KEY:
        {
          extern char cpuBreakLoop;
          extern  void debuggerSignal(int,int);
          debuggerSignal(5,5);
          cpuBreakLoop = true;
          break;
        }
      }
    }
  }
}

@end
