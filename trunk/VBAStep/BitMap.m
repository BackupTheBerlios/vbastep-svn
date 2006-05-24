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

#import "BitMap.h"

static inline unsigned char mask(unsigned which) {
  unsigned char mask = 1 << (which & 7);
  return mask;
}

@implementation VBBitMap
-(id)initWithLength:(unsigned)length {
  self = [super init];

  length = (length + 7) >> 3;
  data = [[NSMutableData alloc] initWithLength: length];
  cdata = [data mutableBytes];
  return self;
}
-(void)dealloc {
  [data release];
  [super dealloc];
}
-(void)setBit:(unsigned)which {
  unsigned index = which >> 3;
  cdata[index] |= mask(which);
}
-(void)clearBit:(unsigned)which {
  unsigned index = which >> 3;
  cdata[index] &= ~mask(which);
}
-(void)resetBits {
  [data resetBytesInRange: NSMakeRange(0, [data length])];
  cdata = [data mutableBytes];
}
-(BOOL)bitIsSet:(unsigned)which {
  unsigned index = which >> 3;
  return cdata[index] & mask(which);
}
-(void)performSelectorWithSelectedBits:(SEL)sel onObject:(id)obj {
  typedef void (*TargetIMP)(id, SEL, unsigned);
  TargetIMP method;
  unsigned char *cur, *end, bit;
  unsigned index = 0;
  if ([obj respondsToSelector: sel]) {
    method = (TargetIMP)[obj methodForSelector: sel];
    cur = cdata;
    end = cur + [data length];
    while (cur < end) {
      unsigned char value = *(cur++);
      for (bit = 0; bit < 8; bit++) {
        if (value & 1)
          method(obj, sel, index);
        index += 1;
        value >>= 1;
      }
    }
  }
}

-(NSString*)description {
  unsigned cap = 4 + 9*[data length] + 3*[data length]/4;
  NSMutableString *result = [NSMutableString stringWithCapacity: cap];
  unsigned char *cur, *end;
  unsigned index = 0;
  [result appendString: @"< "];
  cur = cdata;
  end = cur + [data length];
  while (cur < end) {
    unsigned char value = *(cur++);
    unsigned mask;
    for (mask = 1; mask < 0x100; mask <<= 1) {
      if (value & mask)
        [result appendString: @"1"];
      else
        [result appendString: @"0"];
    }
    if ((++index) % 4 == 0)
      [result appendString: @"\n  "];
    else
      [result appendString: @" "];
  }
  [result appendString: @" >"];
  return result;
}

- (void)getBytes:(void*)buffer range:(NSRange)range {
  [data getBytes:buffer range:range];
}
- (void*)data{
  return cdata;
}
@end

@implementation BitMapPair
-(id)initWithLength:(unsigned)length {
  self = [super init];

  lock = [[NSLock alloc] init];
  read = [[VBBitMap alloc] initWithLength:length];
  write = [[VBBitMap alloc] initWithLength:length];
  return self;
}
-(void)dealloc {
  [lock release];
  [read release];
  [write release];
  [super dealloc];
}
-(void)setBit:(unsigned)which {
  [lock lock];
  [write setBit:which];
  [lock unlock];
}
-(void)clearBit:(unsigned)which {
  [lock lock];
  [write clearBit:which];
  [lock unlock];
}
-(void)resetBits {
  [lock lock];
  [write resetBits];
  [lock unlock];
}
-(void)swapBitMaps {
  VBBitMap *tmp;
  [lock lock];
  tmp = read;
  read = write;
  write = tmp;
  [write resetBits];
  [lock unlock];
}
-(BOOL)bitIsSet:(unsigned)which {
  BOOL result;

  [lock lock];
  result = [read bitIsSet: which];
  [lock unlock];

  return result;
}
-(void)performSelectorWithSelectedBits:(SEL)sel onObject:(id)obj {
  [lock lock];
  [read performSelectorWithSelectedBits:sel onObject:obj];
  [lock unlock];
}
-(VBBitMap*)readMap { return read; }
-(VBBitMap*)writeMap { return write; }
-(NSString*)description {
  return [NSString stringWithFormat:@"< BitMapPair read:\n%@\n     write:\n%@\n>", read, write];
}
- (void)getBytes:(void*)buffer range:(NSRange)range {
  [read getBytes:buffer range:range];
}
-(void*)data {
  return [read data];
}
@end
