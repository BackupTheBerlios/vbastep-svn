#ifndef BITMAP_H
#define BITMAP_H

#include <string.h>

template <unsigned SIZE>
class BitMap
{
public:
  void setBit(unsigned which) {
    if (which < SIZE)
      data[ which / 8 ] |= (1 << (which % 8));
  }
  void clearBit(unsigned which) {
    if (which < SIZE)
      data[ which /8 ] &= ~ (1 << (which % 8));
  }
  void setBitTo(unsigned which, bool value) {
    if (value)
      setBit(which);
    else
      clearBit(value);
  }
  bool isSet(unsigned which) {
    if ((which < SIZE) &&
        data[ which / 8] & (1 << (which % 8)))
      return 1;
    else
      return 0;
  }
  void reset() { memset(data, 0, (SIZE + 7) / 8); }
private:
  unsigned char data[(SIZE + 7) / 8];
};

#endif
