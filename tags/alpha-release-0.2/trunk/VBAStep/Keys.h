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


#define L_KEY 'a'
#define L_KEY_FLAG 1 << 8;
#define R_KEY 's'
#define R_KEY_FLAG 1 << 9;
#define A_KEY 'z'
#define A_KEY_FLAG 1 << 0;
#define B_KEY 'x'
#define B_KEY_FLAG 1 << 1;
#define START_KEY  '\n'
#define START_KEY_FLAG 1 << 3;
#define SELECT_KEY '\\'
#define SELECT_KEY_FLAG 1 << 2;
#define UP_KEY    NSUpArrowFunctionKey
#define UP_KEY_FLAG 1 << 6;
#define DOWN_KEY  NSDownArrowFunctionKey
#define DOWN_KEY_FLAG 1 << 7;
#define LEFT_KEY  NSLeftArrowFunctionKey
#define LEFT_KEY_FLAG 1 << 5;
#define RIGHT_KEY NSRightArrowFunctionKey
#define RIGHT_KEY_FLAG 1 << 4;

extern unsigned int gsKeyState;
