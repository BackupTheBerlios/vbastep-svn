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
#ifndef VBA_SOUND_H
#define VBA_SOUND_H

#define NR10 0x60
#define NR11 0x62
#define NR12 0x63
#define NR13 0x64
#define NR14 0x65
#define NR21 0x68
#define NR22 0x69
#define NR23 0x6c
#define NR24 0x6d
#define NR30 0x70
#define NR31 0x72
#define NR32 0x73
#define NR33 0x74
#define NR34 0x75
#define NR41 0x78
#define NR42 0x79
#define NR43 0x7c
#define NR44 0x7d
#define NR50 0x80
#define NR51 0x81
#define NR52 0x84
#define SGCNT0_H 0x82
#define FIFOA_L 0xa0
#define FIFOA_H 0xa2
#define FIFOB_L 0xa4
#define FIFOB_H 0xa6

extern void soundTick();
extern void soundShutdown();
extern bool soundInit();
extern void soundPause();
extern void soundResume();
extern void soundEnable(int);
extern void soundDisable(int);
extern int  soundGetEnable();
extern void soundReset();
extern void soundSaveGame(gzFile);
extern void soundReadGame(gzFile, int);
extern void soundEvent(u32, u8);
extern void soundEvent(u32, u16);
extern void soundTimerOverflow(int);
extern void soundSetQuality(int);

//extern int SOUND_TICKS;
extern int SOUND_CLOCK_TICKS;
extern int soundTicks;
extern int soundPaused;
extern bool soundOffFlag;
extern int soundQuality;
extern int soundBufferLen;
extern int soundBufferTotalLen;
extern u32 soundNextPosition;
extern u16 soundFinalWave[1470];
extern int soundVolume;

#endif // VBA_SOUND_H
