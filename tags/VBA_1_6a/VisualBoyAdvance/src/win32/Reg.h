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
#ifndef VBA_REG_H
#define VBA_REG_H

extern bool regEnabled;

char *regQueryStringValue(char *key, char *def);
DWORD regQueryDwordValue(char *key, DWORD def, bool force=false);
BOOL regQueryBinaryValue(char *key, char *value, int count);
void regSetStringValue(char *key,char *value);
void regSetDwordValue(char *key,DWORD value,bool force=false);
void regSetBinaryValue(char *key, char *value, int count);
void regDeleteValue(char *key);
void regInit(const char *);
void regShutdown();
bool regCreateFileType(char *ext, char *type);
bool regAssociateType(char *type, char *desc, char *application);
void regExportSettingsToINI();
#endif // VBA_REG_H
