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
#include <stdlib.h>

/// Maintains to lists (of unsigned integers)
/// One list can have items added to it (presumably a producer thread)
/// while the items in the other list can be read (by a consumer thread)
/// Then the currently read list can be emptied and swapped, so the
/// consumer can access the previously written list. Duplicate elements
/// in the list may or may not be allowed
/// Reads are not sychronized, so swap should only be called by the consumer

@interface PCList : NSObject
{
  unsigned int *lists[2];
  size_t capacities[2];
  size_t counts[2];
  int mutable;
  int immutable;
  NSRecursiveLock *lock;
}

- (id)init;
- (id)initWithCapacity:(int)cap;
/// Ensure that the currently mutable list can hold at least n items
- (void)ensureCapcity:(size_t)n;
/// Add an item to the currently mutable list. May do nothing if the
/// item is already in the list.
- (void)addItem:(unsigned int)x;
/// Returns the size of the immutable list
- (size_t)size;
/// Returns the item at index i in the immutable list
- (unsigned int)itemAtIndex:(size_t)i;
/// Currently mutable list becomes the new immutable list
/// The new mutable list is empty
- (void)swap;

@end
