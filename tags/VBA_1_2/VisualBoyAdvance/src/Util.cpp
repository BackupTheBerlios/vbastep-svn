/*
 * VisualBoyAdvanced - Nintendo Gameboy/GameboyAdvance (TM) emulator
 * Copyrigh(c) 1999-2002 Forgotten (vb@emuhq.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>

extern "C" {
#include <png.h>
}

#include "System.h"
#include "NLS.h"

bool utilWritePNGFile(char *fileName, int w, int h, u8 *pix)
{
  u8 writeBuffer[512 * 3];
  
  FILE *fp = fopen(fileName,"wb");

  if(!fp) {
    systemMessage(MSG_ERROR_CREATING_FILE, "Error creating file %s", fileName);
    return false;
  }
  
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                NULL,
                                                NULL,
                                                NULL);
  if(!png_ptr) {
    fclose(fp);
    return false;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);

  if(!info_ptr) {
    png_destroy_write_struct(&png_ptr,NULL);
    fclose(fp);
    return false;
  }

  if(setjmp(png_ptr->jmpbuf)) {
    png_destroy_write_struct(&png_ptr,NULL);
    fclose(fp);
    return false;
  }

  png_init_io(png_ptr,fp);

  png_set_IHDR(png_ptr,
               info_ptr,
               w,
               h,
               8,
               PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr,info_ptr);

  u8 *b = writeBuffer;

  int sizeX = w;
  int sizeY = h;

  switch(systemColorDepth) {
  case 16:
    {
      u16 *p = (u16 *)(pix+(w+1)*2); // skip first black line
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          u16 v = *p++;
          
          *b++ = ((v >> systemRedShift) & 0x001f) << 3; // R
          *b++ = ((v >> systemGreenShift) & 0x001f) << 3; // G 
          *b++ = ((v >> systemBlueShift) & 0x01f) << 3; // B
        }
        p++; // skip black pixel for filters
        png_write_row(png_ptr,writeBuffer);
        
        b = writeBuffer;
      }
    }
    break;
  case 24:
    {
      u8 *pixU8 = (u8 *)pix;
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          if(systemRedShift < systemBlueShift) {
            *b++ = *pixU8++; // R
            *b++ = *pixU8++; // G
            *b++ = *pixU8++; // B
          } else {
            int blue = *pixU8++;
            int green = *pixU8++;
            int red = *pixU8++;
            
            *b++ = red;
            *b++ = green;
            *b++ = blue;
          }
        }
        png_write_row(png_ptr,writeBuffer);
        
        b = writeBuffer;
      }
    }
    break;
  case 32:
    {
      u32 *pixU32 = (u32 *)pix;
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          u32 v = *pixU32++;
          
          *b++ = ((v >> systemRedShift) & 0x001f) << 3; // R
          *b++ = ((v >> systemGreenShift) & 0x001f) << 3; // G
          *b++ = ((v >> systemBlueShift) & 0x001f) << 3; // B
        }
        
        png_write_row(png_ptr,writeBuffer);
        
        b = writeBuffer;
      }
    }
    break;
  }
  
  png_write_end(png_ptr, info_ptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  fclose(fp);

  return true;  
}

void utilPutDword(u8 *p, u32 value)
{
  *p++ = value & 255;
  *p++ = (value >> 8) & 255;
  *p++ = (value >> 16) & 255;
  *p = (value >> 24) & 255;
}

void utilPutWord(u8 *p, u16 value)
{
  *p++ = value & 255;
  *p = (value >> 8) & 255;
}

bool utilWriteBMPFile(char *fileName, int w, int h, u8 *pix)
{
  u8 writeBuffer[512 * 3];
  
  FILE *fp = fopen(fileName,"wb");

  if(!fp) {
    systemMessage(MSG_ERROR_CREATING_FILE, "Error creating file %s", fileName);
    return false;
  }

  struct {
    u8 ident[2];
    u8 filesize[4];
    u8 reserved[4];
    u8 dataoffset[4];
    u8 headersize[4];
    u8 width[4];
    u8 height[4];
    u8 planes[2];
    u8 bitsperpixel[2];
    u8 compression[4];
    u8 datasize[4];
    u8 hres[4];
    u8 vres[4];
    u8 colors[4];
    u8 importantcolors[4];
    u8 pad[2];
  } bmpheader;
  memset(&bmpheader, 0, sizeof(bmpheader));

  bmpheader.ident[0] = 'B';
  bmpheader.ident[1] = 'M';

  u32 fsz = sizeof(bmpheader) + w*h*3;
  utilPutDword(bmpheader.filesize, fsz);
  utilPutDword(bmpheader.dataoffset, 0x38);
  utilPutDword(bmpheader.headersize, 0x28);
  utilPutDword(bmpheader.width, w);
  utilPutDword(bmpheader.height, h);
  utilPutDword(bmpheader.planes, 1);
  utilPutDword(bmpheader.bitsperpixel, 24);
  utilPutDword(bmpheader.datasize, 3*w*h);

  fwrite(&bmpheader, 1, sizeof(bmpheader), fp);

  u8 *b = writeBuffer;

  int sizeX = w;
  int sizeY = h;

  switch(systemColorDepth) {
  case 16:
    {
      u16 *p = (u16 *)(pix+(w+1)*(h-1)*2); // skip first black line
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          u16 v = *p++;

          *b++ = ((v >> systemBlueShift) & 0x01f) << 3; // B      
          *b++ = ((v >> systemGreenShift) & 0x001f) << 3; // G 
          *b++ = ((v >> systemRedShift) & 0x001f) << 3; // R
        }
        p++; // skip black pixel for filters
        p -= 2*(w+1);
        fwrite(writeBuffer, 1, 3*w, fp);
        
        b = writeBuffer;
      }
    }
    break;
  case 24:
    {
      u8 *pixU8 = (u8 *)pix+3*w*(h-1);
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          if(systemRedShift > systemBlueShift) {
            *b++ = *pixU8++; // B
            *b++ = *pixU8++; // G
            *b++ = *pixU8++; // R           
          } else {
            int red = *pixU8++;
            int green = *pixU8++;
            int blue = *pixU8++;
            
            *b++ = blue;
            *b++ = green;
            *b++ = red;
          }
        }
        pixU8 -= 2*3*w;
        fwrite(writeBuffer, 1, 3*w, fp);
        
        b = writeBuffer;
      }
    }
    break;
  case 32:
    {
      u32 *pixU32 = (u32 *)(pix+4*w*(h-1));
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          u32 v = *pixU32++;

          *b++ = ((v >> systemBlueShift) & 0x001f) << 3; // B     
          *b++ = ((v >> systemGreenShift) & 0x001f) << 3; // G
          *b++ = ((v >> systemRedShift) & 0x001f) << 3; // R
        }

        pixU32 -= 2*w;
        
        fwrite(writeBuffer, 1, 3*w, fp);
        
        b = writeBuffer;
      }
    }
    break;
  }

  fclose(fp);

  return true;
}
