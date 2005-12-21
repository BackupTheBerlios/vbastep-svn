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

#import "PCList.h"

@implementation PCList
- (id)init {
  return [self initWithCapacity: 256];
}

- (id)initWithCapacity:(int)cap {
  int i;
  NSAssert(cap > 0, @"Capacity must be positive");

  self = [super init];
  for (i = 0; i < 2; i++) {
    capacities[i] = cap;
    lists[i] = malloc(cap*sizeof(unsigned int));
    counts[i] = 0;
  }
  mutable = 0;
  immutable = 1;
  lock = [[NSRecursiveLock alloc] init];
  return self;
}

- (void) dealloc {
  int i;
  [lock release];
  for (i = 0; i < 2; i++)
    free( lists[i] );
  [super dealloc];
}

- (void)ensureCapcity:(size_t)n {
  [lock lock];
  if ( capacities[ mutable ] < n ) {
    if ( 2 * capacities [ mutable ] > n ) {
      n = 2 * capacities [ mutable ];
    }
    lists[ mutable ] = realloc(lists[ mutable ],
                               n * sizeof(unsigned int));
    capacities[ mutable ] = n;
  }
  [lock unlock];
}

- (void)addItem:(unsigned int)x {
  int index;
  [lock lock];

  index = counts[ mutable ]++;
  [self ensureCapcity: counts[ mutable ]];
  lists[ mutable ][ index ] = x;

  [lock unlock];
}

- (size_t)size {
  return counts[ immutable ];
}

- (unsigned int)itemAtIndex:(size_t)i {
  NSAssert((i < counts[ immutable ]), 
           ([NSString stringWithFormat:
                        @"Index %d out of range",i]));
  return lists[ immutable ][ i ];
}

- (void)swap {
  [lock lock];
  
  // emtpty the new list
  counts[ immutable ] = 0;
  // toggle mutable and immutable
  mutable ^= 1;
  immutable ^= 1;

  [lock unlock];
}
@end
