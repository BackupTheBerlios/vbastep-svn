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
#ifndef VBA_FLASH_H
#define VBA_FLASH_H

extern void flashSaveGame(gzFile gzFile);
extern void flashReadGame(gzFile gzFile);
extern u8 flashRead(u32 address);
extern void flashWrite(u32 address, u8 byte);
extern u8 flashSaveMemory[0x10000];
extern void flashSaveDecide(u32 address, u8 byte);
extern void flashReset();
#endif // VBA_FLASH_H
