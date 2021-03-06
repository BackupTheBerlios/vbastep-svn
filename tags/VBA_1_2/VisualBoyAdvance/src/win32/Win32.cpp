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
#define DIRECTDRAW_VERSION 0x0700
#define DIRECTINPUT_VERSION 0x0500
#define DIRECTSOUND_VERSION 0x0500
#define JOYCONFIG_MESSAGE (WM_USER + 1000)

#include <windows.h>
#include <ddraw.h>
#include <dinput.h>
#include <dsound.h>
#include <stdio.h>
#include <afxres.h>
#include <commctrl.h>
#include <shlwapi.h>

#include "Reg.h"
#include "../GBA.h"
#include "../NLS.h"
#include "../Globals.h"
#include "WinResUtil.h"
#include "../Sound.h"
#include "../unzip.h"
#include "resource.h"
#include "../gb/GB.h"
#include "../gb/gbCheats.h"
#include "../gb/gbGlobals.h"
#include "../gb/gbPrinter.h"
#include "wavwrite.h"
#include "CommDlg.h"
#include "ExportGSASnapshot.h"
#include "AcceleratorManager.h"
#include "IUpdate.h"
#include <list>

static u8 COPYRIGHT[] = {
  0xa9, 0x96, 0x8c, 0x8a, 0x9e, 0x93, 0xbd, 0x90, 0x86, 0xbe, 0x9b, 0x89,
  0x9e, 0x91, 0x9c, 0x9a, 0xdf, 0xd7, 0xbc, 0xd6, 0xdf, 0xce, 0xc6, 0xc6,
  0xc6, 0xd3, 0xcd, 0xcf, 0xcf, 0xcf, 0xd3, 0xcd, 0xcf, 0xcf, 0xce, 0xdf,
  0x9d, 0x86, 0xdf, 0xb9, 0x90, 0x8d, 0x98, 0x90, 0x8b, 0x8b, 0x9a, 0x91,
  0x00
};

HINSTANCE            ddrawDLL     = NULL;
HINSTANCE            dinputDLL    = NULL;
HINSTANCE            dsoundDLL    = NULL;
LPDIRECTDRAW7        pDirectDraw  = NULL;
LPDIRECTDRAWSURFACE7 ddsPrimary   = NULL;
LPDIRECTDRAWSURFACE7 ddsOffscreen = NULL;
LPDIRECTDRAWCLIPPER  ddsClipper   = NULL;
LPDIRECTSOUND        pDirectSound = NULL;
LPDIRECTSOUNDBUFFER  dsbPrimary   = NULL;
LPDIRECTSOUNDBUFFER  dsbSecondary = NULL;
LPDIRECTSOUNDNOTIFY  dsbNotify    = NULL;
HANDLE               dsbEvent     = NULL;
WAVEFORMATEX         wfx;
HWND hWindow;
HWND hJoyConfig = NULL;
HINSTANCE hInstance = NULL;
HMENU menu;
HWND regDialog = NULL;
HACCEL hAccel = NULL;

BOOL screenSaverState = FALSE;
BOOL screenSaverDisabled = FALSE;
BOOL useOldSync = FALSE;

BOOL soundRecording = FALSE;
char soundRecordName[2048];
CWaveSoundWrite *soundRecorder = NULL;

BOOL recentFreeze = FALSE;
BOOL speedupToggle = FALSE;
BOOL removeIntros = FALSE;
BOOL soundInitialized = FALSE;
BOOL iconic = FALSE;
BOOL paused = FALSE;
BOOL winPauseNextFrame = FALSE;
BOOL vsync = FALSE;
BOOL menuToggle = TRUE;
BOOL useBiosFile = FALSE;
BOOL pauseWhenInactive = TRUE;
int nCmdShow = 0;
UINT configDialogTimerId  = 0;
BOOL ddrawEmulationOnly = FALSE;
BOOL ddrawUseVideoMemory = FALSE;
int threadPriority = 2;
int autoFire = 0;
bool autoFireToggle = false;
BOOL active = TRUE;
BOOL fullScreenStretch = FALSE;
BOOL mode320Available = FALSE;
BOOL mode640Available = FALSE;
BOOL mode800Available = FALSE;
BOOL changingVideoSize = FALSE;
u8 *delta[257*244*2];
void (*filterFunction)(u8*,u32,u8*,u8*,u32,int,int) = NULL;
int filterType = 0;
int filterWidth = 0;
int filterHeight = 0;
int RGB_LOW_BITS_MASK = 0;
int sensorX = 2047;
int sensorY = 2047;
int mouseCounter = 0;
HCURSOR arrow = NULL;
int languageOption = 0;
HINSTANCE languageModule = NULL;
char languageName[4];

bool screenMessage = false;
char screenMessageBuffer[41];
DWORD screenMessageTime = 0;

RECT rect;
RECT dest;

int emulating = 0;
bool debugger = false;

bool systemSoundOn = false;
u32 systemColorMap32[0x10000];
u16 systemColorMap16[0x10000];
u16 systemGbPalette[24];
int systemRedShift = 0;
int systemBlueShift = 0;
int systemGreenShift = 0;
int systemColorDepth = 16;
int systemVerbose = 0;
int systemDebug = 0;

char dir[1024];
char szFile[1024];
char filename[2048];
char winBuffer[1024];
char biosFileName[2048];

int sizeX = 0;
int sizeY = 0;
int surfaceSizeX = 0;
int surfaceSizeY = 0;
int windowPositionX = 0;
int windowPositionY = 0;
int videoOption = 0;
int cartridgeType = 0;
bool disableMMX = false;
bool disableStatusMessage = false;
int winGbPrinterEnabled = 0;
bool autoSaveLoadCheatList = false;
int captureFormat = 0;

std::list<IUpdateListener *> updateList;

extern bool soundEcho;
extern bool soundLowPass;
extern bool soundReverse;

bool (*emuWriteState)(char *) = NULL;
bool (*emuReadState)(char *) = NULL;
bool (*emuWriteBattery)(char *) = NULL;
bool (*emuReadBattery)(char *) = NULL;
void (*emuReset)() = NULL;
void (*emuCleanUp)() = NULL;
bool (*emuWritePNG)(char *) = NULL;
bool (*emuWriteBMP)(char *) = NULL;
void (*emuMain)(int) = NULL;
int emuCount = 0;

extern void remoteStubSignal(int, int);
extern void remoteOutput(char *, u32);
extern void remoteStubMain();
extern void remoteSetProtocol(int);
extern void remoteCleanUp();
extern int remoteSocket;

void winSignal(int,int);
void winOutput(char *, u32);

void (*dbgSignal)(int,int) = winSignal;
void (*dbgOutput)(char *, u32) = winOutput;

CAcceleratorManager winAccelMgr;

struct deviceInfo {
  LPDIRECTINPUTDEVICE device;
  BOOL isPolled;
  int nButtons;
  int nAxes;
  int nPovs;
  BOOL first;
  struct {
    DWORD offset;
    LONG center;
    LONG negative;
    LONG positive;
  } axis[8];
  int needed;
  union {
    UCHAR data[256];
    DIJOYSTATE state;
  };
};

deviceInfo *currentDevice = NULL;
int numDevices = 1;
LPDIRECTINPUT        pDirectInput = NULL;
deviceInfo          *pDevices     = NULL;

#define CHECKMENUSTATE(f) ((f) ? MF_CHECKED|MF_BYCOMMAND : \
   MF_UNCHECKED|MF_BYCOMMAND)
#define ENABLEMENU(f) ((f) ? MF_ENABLED|MF_BYCOMMAND : \
   MF_GRAYED|MF_BYCOMMAND)

#define POV_UP    1
#define POV_DOWN  2
#define POV_RIGHT 4
#define POV_LEFT  8

enum {
  VIDEO_1X, VIDEO_2X, VIDEO_3X, VIDEO_4X,
  VIDEO_320x240, VIDEO_640x480, VIDEO_800x600
};

enum {
  KEY_LEFT, KEY_RIGHT,
  KEY_UP, KEY_DOWN,
  KEY_BUTTON_A, KEY_BUTTON_B,
  KEY_BUTTON_START, KEY_BUTTON_SELECT,
  KEY_BUTTON_L, KEY_BUTTON_R,
  KEY_BUTTON_SPEED, KEY_BUTTON_CAPTURE,
  KEY_BUTTON_GS
};

enum {
  MENU_FILE, MENU_OPTIONS, MENU_CHEATS, MENU_TOOLS
};

enum {
  MENU_OPTIONS_FRAME_SKIP,
  MENU_OPTIONS_VIDEO,
  MENU_OPTIONS_EMULATOR,
  MENU_OPTIONS_SOUND,
  MENU_OPTIONS_GAMEBOY,
  MENU_OPTIONS_PRIORITY,
  MENU_OPTIONS_FILTER,
  MENU_OPTIONS_JOYPAD,
  MENU_OPTIONS_LANGUAGE
};

USHORT joypad[13] = {
  DIK_LEFT,  DIK_RIGHT,
  DIK_UP,    DIK_DOWN,
  DIK_Z,     DIK_X,
  DIK_RETURN,DIK_BACK,
  DIK_A,     DIK_S,
  DIK_SPACE, DIK_F12,
  DIK_C
};

USHORT motion[4] = {
  DIK_NUMPAD4, DIK_NUMPAD6, DIK_NUMPAD8, DIK_NUMPAD2
};

char *recentFiles[10];

void adjustDestRect();
void screenCapture();
void winSaveCheatList();
void winSaveCheatListDefault();
void winLoadCheatList();
void winLoadCheatListDefault();
void fileExit();
void fileToggleMenu();
BOOL fileOpen();
BOOL fileOpenSelect();
BOOL fileOpenSelectGB();
void fileSoundRecord();
void loadSaveGame();
void loadSaveGame(int);
void writeSaveGame();
void writeSaveGame(int);
void writeBatteryFile();
void readBatteryFile();
BOOL initDirectDraw();
BOOL initDirectInput();
void winCenterWindow(HWND);
extern int Init_2xSaI(u32);
extern void Pixelate(u8*,u32,u8*,u8*,u32,int,int);
extern void MotionBlur(u8*,u32,u8*,u8*,u32,int,int);
extern void TVMode(u8*,u32,u8*,u8*,u32,int,int);
extern void _2xSaI(u8*,u32,u8*,u8*,u32,int,int);
extern void Super2xSaI(u8*,u32,u8*,u8*,u32,int,int);
extern void SuperEagle(u8*,u32,u8*,u8*,u32,int,int);
extern void AdMame2x(u8*,u32,u8*,u8*,u32,int,int);
extern void Simple2x(u8*,u32,u8*,u8*,u32,int,int);
extern void winGBARomInfo(u8*);
extern void winGBRomInfo(u8*);
extern void winCheatsDialog();
extern void winCheatsListDialog();
extern void showDirectories(HWND);
extern void winGbCheatsDialog();
extern void winGbCheatsListDialog();
extern void configurePad();
extern void motionConfigurePad();
extern int winGSACodeSelect(HWND, LPARAM);
extern void fileExportSPSSnapshot(char *, char *);

#ifdef MMX
extern "C" bool cpu_mmx;

bool detectMMX()
{
  bool support = false;
  char brand[12];

  // check for Intel chip
  __try {
    __asm {
      mov eax, 0;
      cpuid;
      mov [dword ptr brand+0], ebx;
      mov [dword ptr brand+4], edx;
      mov [dword ptr brand+8], ecx;
    }
  }
  __except(EXCEPTION_EXECUTE_HANDLER) {
    if(_exception_code() == STATUS_ILLEGAL_INSTRUCTION) {
      return false;
    }
    return false;
  }
  if(strncmp(brand, "GenuineIntel", 12)) {
    return false;
  }

  __asm {
    mov eax, 1;
    cpuid;
    test edx, 00800000h;
    jz NotFound;
    mov [support], 1;
  NotFound:
  }
  return support;
}
#endif

bool isDriveRoot(char *s)
{
  if(strlen(s) == 3) {
    if(s[1] == ':' && s[2] == '\\')
      return true;
  }
  return false;
}

BOOL getScreenSaverEnable()
{
  if(SystemParametersInfo(SPI_GETSCREENSAVEACTIVE,
                          0,
                          &screenSaverState,
                          0))
    return screenSaverState;
  return FALSE;
}

void setScreenSaverEnable(BOOL e)
{
  screenSaverDisabled = e;

  SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,
                       e,
                       NULL,
                       SPIF_SENDWININICHANGE);  
}

void winMouseOn()
{
  SetCursor(arrow);
  if(videoOption > VIDEO_4X) {
    mouseCounter = 120;
  } else
    mouseCounter = 0;
}

void shutdownDirectInput()
{
  if(pDirectInput != NULL) {
    if(pDevices) {
      for(int i = 0; i < numDevices ; i++) {
        if(pDevices[i].device) {
          pDevices[i].device->Unacquire();
          pDevices[i].device->Release();
          pDevices[i].device = NULL;
        }
      }
      free(pDevices);
      pDevices = NULL;
    }
    
    pDirectInput->Release();
    pDirectInput = NULL;
  }

  if(dinputDLL != NULL) {
    FreeLibrary(dinputDLL);
    dinputDLL = NULL;
  }
}

void shutdownDirectDraw()
{
  if(pDirectDraw != NULL) {
    if(ddsClipper != NULL) {
      ddsClipper->Release();
      ddsClipper = NULL;
    }

    if(ddsOffscreen != NULL) {
      ddsOffscreen->Release();
      ddsOffscreen = NULL;
    }
    
    if(ddsPrimary != NULL) {
      ddsPrimary->Release();
      ddsPrimary = NULL;
    }
    
    pDirectDraw->Release();
    pDirectDraw = NULL;
  }

  if(ddrawDLL != NULL) {
    FreeLibrary(ddrawDLL);
    ddrawDLL = NULL;
  }
}

void releaseAllObjects()
{
  regShutdown();
  
  shutdownDirectInput();
  
  shutdownDirectDraw();

  soundShutdown();

  RedrawWindow(NULL,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
}

int getPovState(DWORD value)
{
  int state = 0;
  if(LOWORD(value) != 0xFFFF) {
    if(value < 9000 || value > 27000)
      state |= POV_UP;
    if(value > 0 && value < 18000)
      state |= POV_RIGHT;
    if(value > 9000 && value < 27000)
      state |= POV_DOWN;
    if(value > 18000)
      state |= POV_LEFT;
  }
  return state;
}

void checkKeys()
{
  int dev = 0;

  for(int i = 0; i < numDevices; i++)
    pDevices[i].needed = 0;
  
  dev = joypad[KEY_LEFT] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    joypad[KEY_LEFT] = DIK_LEFT;
  
  dev = joypad[KEY_RIGHT] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    joypad[KEY_RIGHT] = DIK_RIGHT;
  
  dev = joypad[KEY_UP] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    joypad[KEY_UP] = DIK_UP;
  
  dev = joypad[KEY_DOWN] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    joypad[KEY_DOWN] = DIK_DOWN;
  
  dev = joypad[KEY_BUTTON_A] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    joypad[KEY_BUTTON_A] = DIK_Z;
  
  dev = joypad[KEY_BUTTON_B] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    joypad[KEY_BUTTON_B] = DIK_X;

  dev = joypad[KEY_BUTTON_L] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    joypad[KEY_BUTTON_L] = DIK_A;
  
  dev = joypad[KEY_BUTTON_R] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    joypad[KEY_BUTTON_R] = DIK_S;
  
  dev = joypad[KEY_BUTTON_START] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    joypad[KEY_BUTTON_START] = DIK_RETURN;
  
  dev = joypad[KEY_BUTTON_SELECT] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    joypad[KEY_BUTTON_SELECT] = DIK_BACK;
  
  dev = joypad[KEY_BUTTON_SPEED] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    joypad[KEY_BUTTON_SPEED] = DIK_SPACE;
  
  dev = joypad[KEY_BUTTON_CAPTURE] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    joypad[KEY_BUTTON_CAPTURE] = DIK_F12;

  dev = joypad[KEY_BUTTON_GS] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    joypad[KEY_BUTTON_GS] = DIK_C;
  
  dev = motion[KEY_UP] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    motion[KEY_UP] = DIK_NUMPAD8;  

  dev = motion[KEY_DOWN] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    motion[KEY_DOWN] = DIK_NUMPAD2;  

  dev = motion[KEY_LEFT] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    motion[KEY_LEFT] = DIK_NUMPAD4;  

  dev = motion[KEY_RIGHT] >> 8;
  if(dev < numDevices && dev >= 0)
    pDevices[dev].needed = 1;
  else
    motion[KEY_RIGHT] = DIK_NUMPAD6;  
}

#define KEYDOWN(buffer,key) (buffer[key] & 0x80)

int readKeyboard()
{
  if(pDevices[0].needed) {
    HRESULT hret = pDevices[0].device->
      GetDeviceState(256,
                     (LPVOID)pDevices[0].data);
    
    if(hret == DIERR_INPUTLOST || hret == DIERR_NOTACQUIRED) {
      hret = pDevices[0].device->Acquire();
      if(hret != DI_OK)
        return 0;
      hret = pDevices[0].device->GetDeviceState(256,(LPVOID)pDevices[0].data);
    }
 
    return hret == DI_OK;
  }
  return TRUE;
}

int readJoystick(int joy)
{
  if(pDevices[joy].needed) {
    if(pDevices[joy].isPolled)
      ((LPDIRECTINPUTDEVICE2)pDevices[joy].device)->Poll();
    
    HRESULT hret = pDevices[joy].device->
      GetDeviceState(sizeof(DIJOYSTATE),
                     (LPVOID)&pDevices[joy].state);
    
    if(hret == DIERR_INPUTLOST || hret == DIERR_NOTACQUIRED) {
      hret = pDevices[joy].device->Acquire();
      
      if(hret == DI_OK) {
        
        if(pDevices[joy].isPolled)
          ((LPDIRECTINPUTDEVICE2)pDevices[joy].device)->Poll();
        
        hret = pDevices[joy].device->
          GetDeviceState(sizeof(DIJOYSTATE),
                         (LPVOID)&pDevices[joy].state);
      }
    }

    return hret == DI_OK;
  }

  return TRUE;
}

void checkKeyboard()
{
  HRESULT hret = pDevices[0].device->Acquire();  
  hret = pDevices[0].device->
    GetDeviceState(256,
                   (LPVOID)pDevices[0].data);
  
  if(hret == DIERR_INPUTLOST || hret == DIERR_NOTACQUIRED) {
    return;
  }
 
  if(hret == DI_OK) {
    for(int i = 0; i < 256; i++) {
      if(KEYDOWN(pDevices[0].data, i)) {
        SendMessage(GetFocus(), JOYCONFIG_MESSAGE,0,i);
        break;
      }
    }
  }
}

void setDeviceFirst()
{
  for(int i = 0; i < numDevices; i++)
    pDevices[i].first = TRUE;
}

void checkJoypads()
{
  DIDEVICEOBJECTINSTANCE di;

  ZeroMemory(&di,sizeof(DIDEVICEOBJECTINSTANCE));

  di.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);

  int i =0;

  DIJOYSTATE joystick;
  
  for(i = 1; i < numDevices; i++) {
    HRESULT hret = pDevices[i].device->Acquire();
    

    if(pDevices[i].isPolled)
      ((LPDIRECTINPUTDEVICE2)pDevices[i].device)->Poll();
    
    hret = pDevices[i].device->GetDeviceState(sizeof(joystick), &joystick);

    int j;

    if(pDevices[i].first) {
      memcpy(&pDevices[i].state, &joystick, sizeof(joystick));
      pDevices[i].first = FALSE;
      continue;
    }
    
    for(j = 0; j < pDevices[i].nButtons; j++) {
      if(joystick.rgbButtons[j] & 0x80) {
        HWND focus = GetFocus();

        SendMessage(focus, JOYCONFIG_MESSAGE, i,j+128);
      }      
    }

    for(j = 0; j < pDevices[i].nAxes && j < 8; j++) {
      LONG value = pDevices[i].axis[j].center;
      LONG old = 0;
      switch(pDevices[i].axis[j].offset) {
      case DIJOFS_X:
        value = joystick.lX;
        old = pDevices[i].state.lX;
        break;
      case DIJOFS_Y:
        value = joystick.lY;
        old = pDevices[i].state.lY;     
        break;
      case DIJOFS_Z:
        value = joystick.lZ;
        old = pDevices[i].state.lZ;     
        break;
      case DIJOFS_RX:
        value = joystick.lRx;
        old = pDevices[i].state.lRx;    
        break;
      case DIJOFS_RY:
        value = joystick.lRy;
        old = pDevices[i].state.lRy;    
        break;
      case DIJOFS_RZ:
        value = joystick.lRz;
        old = pDevices[i].state.lRz;    
        break;
      case DIJOFS_SLIDER(0):
        value = joystick.rglSlider[0];
        old = pDevices[i].state.rglSlider[0];   
        break;
      case DIJOFS_SLIDER(1):
        value = joystick.rglSlider[1];
        old = pDevices[i].state.rglSlider[1];   
        break;
      }
      if(!pDevices[i].first && value != old) {
        if(value < pDevices[i].axis[j].negative)
          SendMessage(GetFocus(), JOYCONFIG_MESSAGE, i, (j<<1));
        else if (value > pDevices[i].axis[j].positive)
          SendMessage(GetFocus(), JOYCONFIG_MESSAGE, i, (j<<1)+1);
      }
    }

    for(j = 0;j < 4 && j < pDevices[i].nPovs; j++) {
      int state = getPovState(joystick.rgdwPOV[j]);
      
      if(state & POV_UP)
        SendMessage(GetFocus(), JOYCONFIG_MESSAGE, i, (j<<2)+0x20);
      else if(state & POV_DOWN)
        SendMessage(GetFocus(), JOYCONFIG_MESSAGE, i, (j<<2)+0x21);
      else if(state & POV_RIGHT)
        SendMessage(GetFocus(), JOYCONFIG_MESSAGE, i, (j<<2)+0x22);
      else if(state & POV_LEFT)
        SendMessage(GetFocus(), JOYCONFIG_MESSAGE, i, (j<<2)+0x23);
    }
  }
}

void saveKeys()
{
  regSetDwordValue("Joy0_Left",
                   joypad[KEY_LEFT]);
  regSetDwordValue("Joy0_Right",
                   joypad[KEY_RIGHT]);
  regSetDwordValue("Joy0_Up",
                   joypad[KEY_UP]);
  regSetDwordValue("Joy0_Speed",
                   joypad[KEY_BUTTON_SPEED]);
  regSetDwordValue("Joy0_Capture",
                   joypad[KEY_BUTTON_CAPTURE]);
  regSetDwordValue("Joy0_GS",
                   joypad[KEY_BUTTON_GS]);  
  regSetDwordValue("Joy0_Down",
                   joypad[KEY_DOWN]);
  regSetDwordValue("Joy0_A",
                   joypad[KEY_BUTTON_A]);
  regSetDwordValue("Joy0_B",
                   joypad[KEY_BUTTON_B]);
  regSetDwordValue("Joy0_L",
                   joypad[KEY_BUTTON_L]);
  regSetDwordValue("Joy0_R",
                   joypad[KEY_BUTTON_R]);  
  regSetDwordValue("Joy0_Start",
                   joypad[KEY_BUTTON_START]);
  regSetDwordValue("Joy0_Select",
                   joypad[KEY_BUTTON_SELECT]);
  regSetDwordValue("joyVersion", 1);  
}

void convertKeys()
{
  for(int i = 0 ; i < 13; i++) {
    int k = joypad[i];

    int dev = k >> 8;

    if(dev > 0) {
      k = k & 255;

      if(k >= 4) {
        k = 0x80 + k - 4;
        joypad[i] = ((dev) << 8) | k;
      }
    }
  }
  saveKeys();
}

char *getKeyName(int key)
{
  int d = (key >> 8);
  int k = key & 255;

  DIDEVICEOBJECTINSTANCE di;

  ZeroMemory(&di,sizeof(DIDEVICEOBJECTINSTANCE));

  di.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);
  
  strcpy(winBuffer,winResLoadString(IDS_ERROR));
  
  if(d == 0) {
    pDevices[0].device->GetObjectInfo(&di,key,DIPH_BYOFFSET);
    strcpy(winBuffer,di.tszName);
  } else {
    if(k < 16) {
      if(k < 4) {
        switch(k) {
        case 0:
          wsprintf(winBuffer, winResLoadString(IDS_JOY_LEFT), d);
          break;
        case 1:
          wsprintf(winBuffer, winResLoadString(IDS_JOY_RIGHT), d);
          break;
        case 2:
          wsprintf(winBuffer, winResLoadString(IDS_JOY_UP), d);
          break;
        case 3:
          wsprintf(winBuffer, winResLoadString(IDS_JOY_DOWN), d);
          break;
        }
      } else {
        pDevices[d].device->GetObjectInfo(&di,
                                          pDevices[d].axis[k>>1].offset,
                                          DIPH_BYOFFSET);
        if(k & 1)
          wsprintf(winBuffer,"Joy %d %s +", d, di.tszName);
        else
          wsprintf(winBuffer,"Joy %d %s -", d, di.tszName);
      }
    } else if(k < 48) {
      int hat = (k >> 2) & 3;
      pDevices[d].device->GetObjectInfo(&di,
                                        DIJOFS_POV(hat),
                                        DIPH_BYOFFSET);
      char *dir = "up";
      int dd = k & 3;
      if(dd == 1)
        dir = "down";
      else if(dd == 2)
        dir = "right";
      else if(dd == 3)
        dir = "left";
      wsprintf(winBuffer," Joy %d %s %s", d, di.tszName, dir);
    } else {
      pDevices[d].device->GetObjectInfo(&di,
                                        DIJOFS_BUTTON(k-128),
                                        DIPH_BYOFFSET);
      wsprintf(winBuffer,winResLoadString(IDS_JOY_BUTTON),d,di.tszName);
    }
  }

  return winBuffer;
}

void readKeys()
{
  int key = -1;

  // joy 0
  key = regQueryDwordValue("Joy0_Left", (DWORD)-1);
  if(key != -1)
    joypad[KEY_LEFT] = key;
  key = regQueryDwordValue("Joy0_Right", (DWORD)-1);
  if(key != -1)
    joypad[KEY_RIGHT] = key;
  key = regQueryDwordValue("Joy0_Up", (DWORD)-1);
  if(key != -1)
    joypad[KEY_UP] = key;
  key = regQueryDwordValue("Joy0_Down", (DWORD)-1);
  if(key != -1)
    joypad[KEY_DOWN] = key;
  key = regQueryDwordValue("Joy0_A", (DWORD)-1);
  if(key != -1)
    joypad[KEY_BUTTON_A] = key;
  key = regQueryDwordValue("Joy0_B", (DWORD)-1);
  if(key != -1)
    joypad[KEY_BUTTON_B] = key;
  key = regQueryDwordValue("Joy0_L", (DWORD)-1);
  if(key != -1)
    joypad[KEY_BUTTON_L] = key;
  key = regQueryDwordValue("Joy0_R", (DWORD)-1);
  if(key != -1)
    joypad[KEY_BUTTON_R] = key;  
  key = regQueryDwordValue("Joy0_Start", (DWORD)-1);
  if(key != -1)
    joypad[KEY_BUTTON_START] = key;
  key = regQueryDwordValue("Joy0_Select", (DWORD)-1);
  if(key != -1)
    joypad[KEY_BUTTON_SELECT] = key;
  key = regQueryDwordValue("Joy0_Speed", (DWORD)-1);
  if(key != -1)
    joypad[KEY_BUTTON_SPEED] = key;
  key = regQueryDwordValue("Joy0_Capture", (DWORD)-1);
  if(key != -1)
    joypad[KEY_BUTTON_CAPTURE] = key;
  key = regQueryDwordValue("Joy0_GS", (DWORD)-1);
  if(key != -1)
    joypad[KEY_BUTTON_GS] = key;  
  key = regQueryDwordValue("Motion_Left", (DWORD)-1);
  if(key != -1)
    motion[KEY_LEFT] = key;
  key = regQueryDwordValue("Motion_Right", (DWORD)-1);
  if(key != -1)
    motion[KEY_RIGHT] = key;
  key = regQueryDwordValue("Motion_Up", (DWORD)-1);
  if(key != -1)
    motion[KEY_UP] = key;
  key = regQueryDwordValue("Motion_Down", (DWORD)-1);
  if(key != -1)
    motion[KEY_DOWN] = key;
}

void resetRecentList()
{
  int i = 0;
  for(i = 0; i < 10; i++)
    recentFiles[i] = NULL;
  char buffer[20];

  for(i = 0; i < 10; i++) {
    sprintf(buffer, "recent%d", i);
    regDeleteValue(buffer);
  }
}

void addRecentFile(char *s)
{
  // Do not change recent list if frozen
  if(recentFreeze)
    return;
  int i = 0;
  char buffer[50];
  for(i = 0; i < 10; i++) {
    if(recentFiles[i] == NULL)
      break;
    
    if(strcmp(recentFiles[i], s) == 0) {
      if(i == 0)
        return;
      char *p = recentFiles[i];
      for(int j = i; j > 0; j--) {
        recentFiles[j] = recentFiles[j-1];
      }
      recentFiles[0] = p;
      for(i = 0; i < 10; i++) {
        if(recentFiles[i] == NULL)
          break;
        sprintf(buffer,"recent%d",i);
        regSetStringValue(buffer, recentFiles[i]);
      }
      return;
    }
  }
  int num = 0;
  for(i = 0; i < 10; i++) {
    if(recentFiles[i] != NULL)
      num++;
  }
  if(num == 10) {
    free(recentFiles[9]);
    num--;
  }

  for(i = num; i >= 1; i--) {
    recentFiles[i] = recentFiles[i-1];
  }
  recentFiles[0] = strdup(s);
  for(i = 0; i < 10; i++) {
    if(recentFiles[i] == NULL)
      break;
    sprintf(buffer,"recent%d",i);
    regSetStringValue(buffer, recentFiles[i]);
  }  
}

void updatePriority()
{
  switch(threadPriority) {
  case 0:
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    break;
  case 1:
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);    
    break;
  case 3:
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);    
    break;
  default:
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
  }
}

void updateMenuBar()
{
  if(menu != NULL)
    DestroyMenu(menu);
  
  menu = winResLoadMenu(MAKEINTRESOURCE(IDR_MENU));     
  
  SetMenu(hWindow,menu);
}

HINSTANCE winLoadLanguage(char *name)
{
  char buffer[20];
  
  sprintf(buffer, "vba_%s.dll", name);

  HINSTANCE l = LoadLibrary(buffer);
  
  if(l == NULL) {
    if(strlen(name) == 3) {
      char buffer2[3];
      buffer2[0] = name[0];
      buffer2[1] = name[1];
      buffer2[2] = 0;
      sprintf(buffer, "vba_%s.dll", buffer2);

      return LoadLibrary(buffer);
    }
  }
  return l;
}

extern int languageSelect();

void winSetLanguageOption(int option, bool force)
{
  if(((option == languageOption) && option != 2) && !force)
    return;
  switch(option) {
  case 0:
    {
      char lbuffer[10];

      if(GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SABBREVLANGNAME,
                       lbuffer, 10)) {
        HINSTANCE l = winLoadLanguage(lbuffer);
        if(l == NULL) {
          systemMessage(IDS_FAILED_TO_LOAD_LIBRARY,
                        "Failed to load library %s",
                        lbuffer);
          return;
        }
        winResSetLanguageModule(l);
        if(languageModule != NULL)
          FreeLibrary(languageModule);
        languageModule = l;
      } else {
        systemMessage(IDS_FAILED_TO_GET_LOCINFO,
                      "Failed to get locale information");
        return;
      }
    }
    break;
  case 1:
    if(languageModule != NULL)
      FreeLibrary(languageModule);
    languageModule = NULL;
    winResSetLanguageModule(NULL);
    break;
  case 2:
    {
      if(!force) {
        if(languageSelect()) {
          HINSTANCE l = winLoadLanguage(languageName);
          if(l == NULL) {
            systemMessage(IDS_FAILED_TO_LOAD_LIBRARY,
                          "Failed to load library %s",
                          languageName);
            return;
          }
          winResSetLanguageModule(l);
          if(languageModule != NULL)
            FreeLibrary(languageModule);
          languageModule = l;
        }
      } else {
        if(!languageName[0])
          return;
        HINSTANCE l = winLoadLanguage(languageName);
        if(l == NULL) {
          systemMessage(IDS_FAILED_TO_LOAD_LIBRARY,
                        "Failed to load library %s",
                        languageName);
          return;
        }
        winResSetLanguageModule(l);
        if(languageModule != NULL)
          FreeLibrary(languageModule);
        languageModule = l;
      }
    }
    break;
  }
  languageOption = option;
  regSetDwordValue("language", option);
  if(option == 2)
    regSetStringValue("languageName", languageName);
  updateMenuBar();
}

void updateImportMenu(HMENU menu)
{
  if(menu == NULL)
    return;
  
  EnableMenuItem(menu, ID_FILE_IMPORT_BATTERYFILE,
                 ENABLEMENU(emulating));
  EnableMenuItem(menu, ID_FILE_IMPORT_GAMESHARKSNAPSHOT,
                 ENABLEMENU(emulating));
  EnableMenuItem(menu, ID_FILE_IMPORT_GAMESHARKCODEFILE,
                 ENABLEMENU(emulating));
}  

void updateExportMenu(HMENU menu)
{
  if(menu == NULL)
    return;
  
  EnableMenuItem(menu, ID_FILE_EXPORT_BATTERYFILE,
                 ENABLEMENU(emulating));
  EnableMenuItem(menu, ID_FILE_EXPORT_GAMESHARKSNAPSHOT,
                 ENABLEMENU(emulating && cartridgeType == 0));
}  

void updateRecentMenu(HMENU menu)
{
  if(menu == NULL)
    return;
  
  CheckMenuItem(menu, ID_FILE_RECENT_FREEZE,
                CHECKMENUSTATE(recentFreeze));
  
  int i;
  for(i = 0; i < 10; i++) {
    if(!RemoveMenu(menu, ID_FILE_MRU_FILE1+i, MF_BYCOMMAND))
      break;
  }
  for(i = 0; i < 10; i++) {
    char *p = recentFiles[i];
    if(p == NULL)
      break;
    char *l = strrchr(p, '\\');
    if(l != NULL)
      p = l + 1;

    char buffer[2048];

    sprintf(buffer, "%s\tCtrl+F%d", p, i+1);
    
    AppendMenu(menu, MF_STRING, ID_FILE_MRU_FILE1+i, buffer);
  }
}

void updateFileMenu(HMENU menu)
{
  EnableMenuItem(menu, ID_FILE_LOAD,
                 ENABLEMENU(emulating));
  EnableMenuItem(menu, ID_FILE_SAVE,
                 ENABLEMENU(emulating));
  EnableMenuItem(menu, ID_FILE_PAUSE,
                 ENABLEMENU(emulating));
  EnableMenuItem(menu, ID_FILE_RESET,
                 ENABLEMENU(emulating));
  EnableMenuItem(menu, ID_FILE_CLOSE,
                 ENABLEMENU(emulating));
  
  CheckMenuItem(menu, ID_FILE_PAUSE,
                CHECKMENUSTATE(paused));

  EnableMenuItem(menu, ID_FILE_SCREENCAPTURE,
                 ENABLEMENU(emulating));
  
  EnableMenuItem(menu, ID_FILE_ROMINFORMATION,
                 ENABLEMENU(emulating && videoOption != VIDEO_320x240));
  
  HMENU load = GetSubMenu(menu, 4);
  int offset = 0;
  if(load == NULL)
    offset = 1;
  load = GetSubMenu(menu, 4+offset);
  if(load != NULL) {
    for(int i = 0; i < 10; i++)
      EnableMenuItem(load, ID_FILE_LOADGAME_SLOT1+i,
                     ENABLEMENU(emulating));
  }
  HMENU save = GetSubMenu(menu, 5+offset);
  if(save != NULL) {
    for(int i = 0; i < 10; i++)
      EnableMenuItem(save, ID_FILE_SAVEGAME_SLOT1+i,
                     ENABLEMENU(emulating));
  }

  updateRecentMenu(GetSubMenu(menu, 10+offset));
  updateImportMenu(GetSubMenu(menu, 12+offset));
  updateExportMenu(GetSubMenu(menu, 13+offset));
}

void updateFrameSkipMenu(HMENU menu)
{
  menu = GetSubMenu(menu, MENU_OPTIONS_FRAME_SKIP);

  if(menu == NULL)
    return;

  if(cartridgeType == 0) {
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_0,
                  CHECKMENUSTATE(frameSkip == 0));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_1,
                  CHECKMENUSTATE(frameSkip == 1));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_2,
                  CHECKMENUSTATE(frameSkip == 2));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_3,
                  CHECKMENUSTATE(frameSkip == 3));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_4,
                  CHECKMENUSTATE(frameSkip == 4));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_5,
                  CHECKMENUSTATE(frameSkip == 5));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_6,
                  CHECKMENUSTATE(frameSkip == 6));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_7,
                  CHECKMENUSTATE(frameSkip == 7));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_8,
                  CHECKMENUSTATE(frameSkip == 8));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_9,
                  CHECKMENUSTATE(frameSkip == 9));
  } else {
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_0,
                  CHECKMENUSTATE(gbFrameSkip == 0));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_1,
                  CHECKMENUSTATE(gbFrameSkip == 1));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_2,
                  CHECKMENUSTATE(gbFrameSkip == 2));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_3,
                  CHECKMENUSTATE(gbFrameSkip == 3));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_4,
                  CHECKMENUSTATE(gbFrameSkip == 4));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_5,
                  CHECKMENUSTATE(gbFrameSkip == 5));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_6,
                  CHECKMENUSTATE(gbFrameSkip == 6));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_7,
                  CHECKMENUSTATE(gbFrameSkip == 7));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_8,
                  CHECKMENUSTATE(gbFrameSkip == 8));
    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FRAMESKIP_9,
                  CHECKMENUSTATE(gbFrameSkip == 9));
  }
}

void updateLayersMenu(HMENU menu)
{
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_LAYERS_BG0,
                CHECKMENUSTATE(layerSettings & 0x0100));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_LAYERS_BG1,
                CHECKMENUSTATE(layerSettings & 0x0200));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_LAYERS_BG2,
                CHECKMENUSTATE(layerSettings & 0x0400));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_LAYERS_BG3,
                CHECKMENUSTATE(layerSettings & 0x0800));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_LAYERS_OBJ,
                CHECKMENUSTATE(layerSettings & 0x1000));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_LAYERS_WIN0,
                CHECKMENUSTATE(layerSettings & 0x2000));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_LAYERS_WIN1,
                CHECKMENUSTATE(layerSettings & 0x4000));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_LAYERS_OBJWIN,
                CHECKMENUSTATE(layerSettings & 0x8000));
}

void updateVideoMenu(HMENU menu)
{
  menu = GetSubMenu(menu, MENU_OPTIONS_VIDEO);
  if(menu == NULL)
    return;
  
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_VSYNC,
                CHECKMENUSTATE(vsync == 1));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_X1,
                CHECKMENUSTATE(videoOption == VIDEO_1X));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_X2,
                CHECKMENUSTATE(videoOption == VIDEO_2X));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_X3,
                CHECKMENUSTATE(videoOption == VIDEO_3X));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_X4,
                CHECKMENUSTATE(videoOption == VIDEO_4X));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_FULLSCREEN320X240,
                CHECKMENUSTATE((videoOption == VIDEO_320x240)));
  EnableMenuItem(menu, ID_OPTIONS_VIDEO_FULLSCREEN320X240,
                 ENABLEMENU(mode320Available));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_FULLSCREEN640X480,
                CHECKMENUSTATE((videoOption == VIDEO_640x480)));
  EnableMenuItem(menu, ID_OPTIONS_VIDEO_FULLSCREEN640X480,
                 ENABLEMENU(mode640Available));  
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_FULLSCREEN800X600,
                CHECKMENUSTATE((videoOption == VIDEO_800x600)));
  EnableMenuItem(menu, ID_OPTIONS_VIDEO_FULLSCREEN800X600,
                 ENABLEMENU(mode800Available));  
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_DISABLESFX,
                CHECKMENUSTATE(cpuDisableSfx));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_FULLSCREENSTRETCHTOFIT,
                CHECKMENUSTATE(fullScreenStretch));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_DDRAWEMULATIONONLY,
                CHECKMENUSTATE(ddrawEmulationOnly));
  CheckMenuItem(menu, ID_OPTIONS_VIDEO_DDRAWUSEVIDEOMEMORY,
                CHECKMENUSTATE(ddrawUseVideoMemory));

  menu = GetSubMenu(menu, 15);
  if(menu != NULL)
    updateLayersMenu(menu);
}

void updateSaveTypeMenu(HMENU menu)
{
  if(menu == NULL)
    return;

  CheckMenuItem(menu, ID_OPTIONS_EMULATOR_SAVETYPE_AUTOMATIC,
                CHECKMENUSTATE(cpuSaveType == 0));
  CheckMenuItem(menu, ID_OPTIONS_EMULATOR_SAVETYPE_EEPROM,
                CHECKMENUSTATE(cpuSaveType == 1));
  CheckMenuItem(menu, ID_OPTIONS_EMULATOR_SAVETYPE_SRAM,
                CHECKMENUSTATE(cpuSaveType == 2));
  CheckMenuItem(menu, ID_OPTIONS_EMULATOR_SAVETYPE_FLASH,
                CHECKMENUSTATE(cpuSaveType == 3));
  CheckMenuItem(menu, ID_OPTIONS_EMULATOR_SAVETYPE_EEPROMSENSOR,
                CHECKMENUSTATE(cpuSaveType == 4));
}

void updateEmulatorMenu(HMENU menu)
{
  menu = GetSubMenu(menu, MENU_OPTIONS_EMULATOR);

  if(menu == NULL)
    return;

  EnableMenuItem(menu, ID_OPTIONS_EMULATOR_DIRECTORIES,
                 ENABLEMENU(videoOption != VIDEO_320x240));
  
  CheckMenuItem(menu, ID_OPTIONS_EMULATOR_SYNCHRONIZE,
                CHECKMENUSTATE(synchronize));
  CheckMenuItem(menu, ID_OPTIONS_EMULATOR_DISABLESTATUSMESSAGES,
                CHECKMENUSTATE(disableStatusMessage));  
  CheckMenuItem(menu, ID_OPTIONS_EMULATOR_USEBIOSFILE,
                CHECKMENUSTATE(useBiosFile));
  CheckMenuItem(menu, ID_OPTIONS_EMULATOR_PAUSEWHENINACTIVE,
                CHECKMENUSTATE(pauseWhenInactive));
  CheckMenuItem(menu, ID_OPTIONS_EMULATOR_SPEEDUPTOGGLE,
                CHECKMENUSTATE(speedupToggle));
  CheckMenuItem(menu, ID_OPTIONS_EMULATOR_REMOVEINTROSGBA,
                CHECKMENUSTATE(removeIntros));
  EnableMenuItem(menu, ID_OPTIONS_EMULATOR_USEBIOSFILE,
                 ENABLEMENU(biosFileName[0]));  

  CheckMenuItem(menu, ID_OPTIONS_EMULATOR_PNGFORMAT,
                CHECKMENUSTATE(captureFormat == 0));
  CheckMenuItem(menu, ID_OPTIONS_EMULATOR_BMPFORMAT,
                CHECKMENUSTATE(captureFormat != 0));

  menu = GetSubMenu(menu, 7);

  updateSaveTypeMenu(menu);
}

void updateSoundMenu(HMENU menu)
{
  int active = soundGetEnable() & 0x30f;

  menu = GetSubMenu(menu,MENU_OPTIONS_SOUND);
  if(menu == NULL)
    return;
  CheckMenuItem(menu, ID_OPTIONS_SOUND_OFF,
                CHECKMENUSTATE(soundOffFlag));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_MUTE,
                CHECKMENUSTATE(active == 0));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_ON,
                CHECKMENUSTATE(active != 0 && !soundOffFlag));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_USEOLDSYNCHRONIZATION,
                CHECKMENUSTATE(useOldSync));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_ECHO,
                CHECKMENUSTATE(soundEcho));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_LOWPASSFILTER,
                CHECKMENUSTATE(soundLowPass));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_REVERSESTEREO,
                CHECKMENUSTATE(soundReverse));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_CHANNEL1,
                CHECKMENUSTATE(active & 1));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_CHANNEL2,
                CHECKMENUSTATE(active & 2));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_CHANNEL3,
                CHECKMENUSTATE(active & 4));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_CHANNEL4,
                CHECKMENUSTATE(active & 8));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_DIRECTSOUNDA,
                CHECKMENUSTATE(active & 256));
  EnableMenuItem(menu, ID_OPTIONS_SOUND_DIRECTSOUNDA,
                 ENABLEMENU(cartridgeType == 0));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_DIRECTSOUNDB,
                CHECKMENUSTATE(active & 512));
  EnableMenuItem(menu, ID_OPTIONS_SOUND_DIRECTSOUNDB,
                 ENABLEMENU(cartridgeType == 0));  
  CheckMenuItem(menu, ID_OPTIONS_SOUND_11KHZ,
                CHECKMENUSTATE(soundQuality == 4));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_22KHZ,
                CHECKMENUSTATE(soundQuality == 2));
  CheckMenuItem(menu, ID_OPTIONS_SOUND_44KHZ,
                CHECKMENUSTATE(soundQuality == 1));
  EnableMenuItem(menu, ID_OPTIONS_SOUND_STARTRECORDING,
                 ENABLEMENU(!soundRecording));
  EnableMenuItem(menu, ID_OPTIONS_SOUND_STOPRECORDING,
                 ENABLEMENU(soundRecording));
}

void updateGameboyMenu(HMENU menu)
{
  menu = GetSubMenu(menu,MENU_OPTIONS_GAMEBOY);

  if(menu == NULL)
    return;
  
  CheckMenuItem(menu, ID_OPTIONS_GAMEBOY_BORDER,
                CHECKMENUSTATE(gbBorderOn));
  CheckMenuItem(menu, ID_OPTIONS_GAMEBOY_PRINTER,
                CHECKMENUSTATE(gbSerialFunction == gbPrinterSend));
  CheckMenuItem(menu, ID_OPTIONS_GAMEBOY_AUTOMATIC,
                CHECKMENUSTATE(gbEmulatorType == 0));
  CheckMenuItem(menu, ID_OPTIONS_GAMEBOY_GBA,
                CHECKMENUSTATE(gbEmulatorType == 4));
  CheckMenuItem(menu, ID_OPTIONS_GAMEBOY_CGB,
                CHECKMENUSTATE(gbEmulatorType == 1));
  CheckMenuItem(menu, ID_OPTIONS_GAMEBOY_SGB,
                CHECKMENUSTATE(gbEmulatorType == 2));
  CheckMenuItem(menu, ID_OPTIONS_GAMEBOY_GB,
                CHECKMENUSTATE(gbEmulatorType == 3));
  CheckMenuItem(menu, ID_OPTIONS_GAMEBOY_REALCOLORS,
                CHECKMENUSTATE(gbColorOption == 0));
  CheckMenuItem(menu, ID_OPTIONS_GAMEBOY_GAMEBOYCOLORS,
                CHECKMENUSTATE(gbColorOption != 0));
}

void updatePriorityMenu(HMENU menu)
{
  menu = GetSubMenu(menu,MENU_OPTIONS_PRIORITY);

  if(menu == NULL)
    return;
  
  CheckMenuItem(menu, ID_OPTIONS_PRIORITY_HIGHEST,
                CHECKMENUSTATE(threadPriority == 0));
  CheckMenuItem(menu, ID_OPTIONS_PRIORITY_ABOVENORMAL,
                CHECKMENUSTATE(threadPriority == 1));
  CheckMenuItem(menu, ID_OPTIONS_PRIORITY_NORMAL,
                CHECKMENUSTATE(threadPriority == 2));
  CheckMenuItem(menu, ID_OPTIONS_PRIORITY_BELOWNORMAL,
                CHECKMENUSTATE(threadPriority == 3));
}

void updateFilterMenu(HMENU menu)
{
  menu = GetSubMenu(menu,MENU_OPTIONS_FILTER);

  if(menu == NULL)
    return;
  
  CheckMenuItem(menu, ID_OPTIONS_FILTER_NORMAL,
                CHECKMENUSTATE(filterType == 0));
  EnableMenuItem(menu, ID_OPTIONS_FILTER_NORMAL,
                 ENABLEMENU(systemColorDepth == 16));
  CheckMenuItem(menu, ID_OPTIONS_FILTER_TVMODE,
                CHECKMENUSTATE(filterType == 1));
  EnableMenuItem(menu, ID_OPTIONS_FILTER_TVMODE,
                 ENABLEMENU(systemColorDepth == 16));
  CheckMenuItem(menu, ID_OPTIONS_FILTER_2XSAI,
                CHECKMENUSTATE(filterType == 2));
  EnableMenuItem(menu, ID_OPTIONS_FILTER_2XSAI,
                 ENABLEMENU(systemColorDepth == 16));
  CheckMenuItem(menu, ID_OPTIONS_FILTER_SUPER2XSAI,
                CHECKMENUSTATE(filterType == 3));
  EnableMenuItem(menu, ID_OPTIONS_FILTER_SUPER2XSAI,
                 ENABLEMENU(systemColorDepth == 16));
  CheckMenuItem(menu, ID_OPTIONS_FILTER_SUPEREAGLE,
                CHECKMENUSTATE(filterType == 4));
  EnableMenuItem(menu, ID_OPTIONS_FILTER_SUPEREAGLE,
                 ENABLEMENU(systemColorDepth == 16));
  CheckMenuItem(menu, ID_OPTIONS_FILTER16BIT_PIXELATEEXPERIMENTAL,
                CHECKMENUSTATE(filterType == 5));
  EnableMenuItem(menu, ID_OPTIONS_FILTER16BIT_PIXELATEEXPERIMENTAL,
                 ENABLEMENU(systemColorDepth == 16));
  CheckMenuItem(menu, ID_OPTIONS_FILTER16BIT_MOTIONBLUREXPERIMENTAL,
                CHECKMENUSTATE(filterType == 6));
  EnableMenuItem(menu, ID_OPTIONS_FILTER16BIT_MOTIONBLUREXPERIMENTAL,
                 ENABLEMENU(systemColorDepth == 16));  
  CheckMenuItem(menu, ID_OPTIONS_FILTER16BIT_ADVANCEMAMESCALE2X,
                CHECKMENUSTATE(filterType == 7));
  EnableMenuItem(menu, ID_OPTIONS_FILTER16BIT_ADVANCEMAMESCALE2X,
                 ENABLEMENU(systemColorDepth == 16));  
  CheckMenuItem(menu, ID_OPTIONS_FILTER16BIT_SIMPLE2X,
                CHECKMENUSTATE(filterType == 8));
  EnableMenuItem(menu, ID_OPTIONS_FILTER16BIT_SIMPLE2X,
                 ENABLEMENU(systemColorDepth == 16));  
  CheckMenuItem(menu, ID_OPTIONS_FILTER_DISABLEMMX,
                CHECKMENUSTATE(disableMMX));
}

void updateJoypadMenu(HMENU menu)
{
  menu = GetSubMenu(menu,MENU_OPTIONS_JOYPAD);
  
  if(menu == NULL)
    return;

  EnableMenuItem(menu, ID_OPTIONS_JOYPAD,
                 ENABLEMENU(videoOption != VIDEO_320x240));
  EnableMenuItem(menu, ID_OPTIONS_JOYPAD_MOTIONCONFIGURE,
                 ENABLEMENU(videoOption != VIDEO_320x240));
  
  menu = GetSubMenu(menu, 3);
  if(menu == NULL)
    return;

  CheckMenuItem(menu, ID_OPTIONS_JOYPAD_AUTOFIRE_A,
                CHECKMENUSTATE(autoFire & 1));
  CheckMenuItem(menu, ID_OPTIONS_JOYPAD_AUTOFIRE_B,
                CHECKMENUSTATE(autoFire & 2));
  CheckMenuItem(menu, ID_OPTIONS_JOYPAD_AUTOFIRE_L,
                CHECKMENUSTATE(autoFire & 512));
  CheckMenuItem(menu, ID_OPTIONS_JOYPAD_AUTOFIRE_R,
                CHECKMENUSTATE(autoFire & 256));
}

void updateLanguageMenu(HMENU menu)
{
  menu = GetSubMenu(menu,MENU_OPTIONS_LANGUAGE);
  
  if(menu == NULL)
    return;

  CheckMenuItem(menu, ID_OPTIONS_LANGUAGE_SYSTEM,
                CHECKMENUSTATE(languageOption == 0));
  CheckMenuItem(menu, ID_OPTIONS_LANGUAGE_ENGLISH,
                CHECKMENUSTATE(languageOption == 1));
  CheckMenuItem(menu, ID_OPTIONS_LANGUAGE_OTHER,
                CHECKMENUSTATE(languageOption == 2));
}

void updateToolsMenu(HMENU menu)
{
  if(menu == NULL)
    return;

  EnableMenuItem(menu, ID_TOOLS_DISASSEMBLE,
                 ENABLEMENU(videoOption <= VIDEO_4X));
  EnableMenuItem(menu, ID_TOOLS_LOGGING,
                 ENABLEMENU(videoOption <= VIDEO_4X));
  EnableMenuItem(menu, ID_TOOLS_MAPVIEW,
                 ENABLEMENU(videoOption <= VIDEO_4X));
  EnableMenuItem(menu, ID_TOOLS_MEMORYVIEWER,
                 ENABLEMENU(videoOption <= VIDEO_4X));
  EnableMenuItem(menu, ID_TOOLS_PALETTEVIEW,
                 ENABLEMENU(videoOption <= VIDEO_4X));
  EnableMenuItem(menu, ID_TOOLS_OAMVIEWER,
                 ENABLEMENU(videoOption <= VIDEO_4X));
  EnableMenuItem(menu, ID_TOOLS_TILEVIEWER,
                 ENABLEMENU(videoOption <= VIDEO_4X));
  EnableMenuItem(menu, ID_TOOLS_CUSTOMIZE,
                 ENABLEMENU(videoOption <= VIDEO_4X));  

  HMENU m = GetSubMenu(menu, 8);
  if(m == NULL)
    m = GetSubMenu(menu, 9);
  menu = m;
  if(menu != NULL) {
    EnableMenuItem(menu, ID_TOOLS_DEBUG_GDB,
                   ENABLEMENU(videoOption <= VIDEO_4X && remoteSocket == -1));
    EnableMenuItem(menu, ID_TOOLS_DEBUG_LOADANDWAIT,
                   ENABLEMENU(videoOption <= VIDEO_4X && remoteSocket == -1));
    EnableMenuItem(menu, ID_TOOLS_DEBUG_DISCONNECT,
                   ENABLEMENU(videoOption <= VIDEO_4X && remoteSocket != -1));
    EnableMenuItem(menu, ID_TOOLS_DEBUG_BREAK,
                   ENABLEMENU(videoOption <= VIDEO_4X && remoteSocket != -1));
  }
}

void updateSoundChannels(UINT id)
{
  int flag = 0;
      
  if(id == ID_OPTIONS_SOUND_CHANNEL1)
    flag = 1;

  if(id == ID_OPTIONS_SOUND_CHANNEL2)
    flag = 2;

  if(id == ID_OPTIONS_SOUND_CHANNEL3)
    flag = 4;

  if(id == ID_OPTIONS_SOUND_CHANNEL4)
    flag = 8;

  if(id == ID_OPTIONS_SOUND_DIRECTSOUNDA)
    flag = 256;

  if(id == ID_OPTIONS_SOUND_DIRECTSOUNDB)
    flag = 512;

  int active = soundGetEnable() & 0x30f;

  if(active & flag)
    active &= (~flag);
  else
    active |= flag;
  
  soundEnable(active);
  soundDisable((~active)&0x30f);
}

void updateCheatsMenu(HMENU menu)
{
  EnableMenuItem(menu, ID_CHEATS_SEARCHFORCHEATS,
                 ENABLEMENU(emulating));
  EnableMenuItem(menu, ID_CHEATS_CHEATLIST,
                 ENABLEMENU(emulating));
  CheckMenuItem(menu, ID_CHEATS_AUTOMATICSAVELOADCHEATS,
                CHECKMENUSTATE(autoSaveLoadCheatList));
  EnableMenuItem(menu, ID_CHEATS_LOADCHEATLIST,
                 ENABLEMENU(emulating));
  EnableMenuItem(menu, ID_CHEATS_SAVECHEATLIST,
                 ENABLEMENU(emulating));
}

int axisNumber = 0;

BOOL CALLBACK EnumPovsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
                               VOID* pContext )
{
  return DIENUM_CONTINUE;  
}

//-----------------------------------------------------------------------------
// Name: EnumAxesCallback()
// Desc: Callback function for enumerating the axes on a joystick
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
                                VOID* pContext )
{
  DIPROPRANGE diprg; 
  diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
  diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
  diprg.diph.dwHow        = DIPH_BYOFFSET; 
  diprg.diph.dwObj        = pdidoi->dwOfs; // Specify the enumerated axis

  diprg.lMin = -32768;
  diprg.lMax = 32767;
  // try to set the range
  if(FAILED(currentDevice->device->SetProperty(DIPROP_RANGE, &diprg.diph))) {
    // Get the range for the axis
    if( FAILED(currentDevice->device->
               GetProperty( DIPROP_RANGE, &diprg.diph ) ) ) {
      return DIENUM_STOP;
    }
  }

  DIPROPDWORD didz;

  didz.diph.dwSize = sizeof(didz);
  didz.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  didz.diph.dwHow = DIPH_BYOFFSET;
  didz.diph.dwObj = pdidoi->dwOfs;

  didz.dwData = 5000;

  currentDevice->device->SetProperty(DIPROP_DEADZONE, &didz.diph);
  
  LONG center = (diprg.lMin + diprg.lMax)/2;
  LONG threshold = (diprg.lMax - center)/2;

  // only 8 axis supported
  if(axisNumber < 8) {
    currentDevice->axis[axisNumber].center = center;
    currentDevice->axis[axisNumber].negative = center - threshold;
    currentDevice->axis[axisNumber].positive = center + threshold;
    currentDevice->axis[axisNumber].offset = pdidoi->dwOfs;
  }
  axisNumber++;
  return DIENUM_CONTINUE;
}

BOOL CALLBACK DIEnumDevicesCallback(LPCDIDEVICEINSTANCE pInst,
                                    LPVOID lpvContext)
{
  ZeroMemory(&pDevices[numDevices],sizeof(deviceInfo));
  
  HRESULT hRet = pDirectInput->CreateDevice(pInst->guidInstance,
                                            &pDevices[numDevices].device,
                                            NULL);
  
  if(hRet != DI_OK)
    return DIENUM_STOP;
  
  DIDEVCAPS caps;
  caps.dwSize=sizeof(DIDEVCAPS);
  
  hRet = pDevices[numDevices].device->GetCapabilities(&caps);
  
  if(hRet == DI_OK) {
    if(caps.dwFlags & DIDC_POLLEDDATAFORMAT ||
       caps.dwFlags & DIDC_POLLEDDEVICE)
      pDevices[numDevices].isPolled = TRUE;
    
    pDevices[numDevices].nButtons = caps.dwButtons;
    pDevices[numDevices].nAxes = caps.dwAxes;
    pDevices[numDevices].nPovs = caps.dwPOVs;

    for(int i = 0; i < 6; i++) {
      pDevices[numDevices].axis[i].center = 0x8000;
      pDevices[numDevices].axis[i].negative = 0x4000;
      pDevices[numDevices].axis[i].positive = 0xc000;
    }
  }

  /*
  if(joydebug) {
    // don't translate. debug only
    log("Joystick name    : %s\n", pInst->tszProductName);
  }
  */
  
  numDevices++;

  
  return DIENUM_CONTINUE;
}

BOOL CALLBACK DIEnumDevicesCallback2(LPCDIDEVICEINSTANCE pInst,
                                     LPVOID lpvContext)
{
  numDevices++;
  
  return DIENUM_CONTINUE;
}

void directXMessage(char *msg)
{
  systemMessage(IDS_DIRECTX_7_REQUIRED,
                "DirectX 7.0 or greater is required to run.\nDownload at http://www.microsoft.com/directx.\n\nError found at: %s",
                msg);
}

BOOL initDirectInput()
{
  dinputDLL = LoadLibrary("DINPUT.DLL");
  HRESULT (WINAPI *DInputCreate)(HINSTANCE,DWORD,LPDIRECTINPUT *,IUnknown *);  
  if(dinputDLL != NULL) {    
    DInputCreate = (HRESULT (WINAPI *)(HINSTANCE,DWORD,LPDIRECTINPUT *,IUnknown *))
      GetProcAddress(dinputDLL, "DirectInputCreateA");
    
    if(DInputCreate == NULL) {
      directXMessage("DirectInputCreateA");
      return FALSE;
    }
  } else {
    directXMessage("DINPUT.DLL");
    return FALSE;
  }
  
  HRESULT hret = DInputCreate(hInstance,
                              DIRECTINPUT_VERSION,
                              &pDirectInput,
                              NULL);
  if(hret != DI_OK) {
    //    errorMessage(myLoadString(IDS_ERROR_DISP_CREATE), hret);
    return FALSE;
  }

  hret = pDirectInput->EnumDevices(DIDEVTYPE_JOYSTICK,
                                   DIEnumDevicesCallback2,
                                   NULL,
                                   DIEDFL_ATTACHEDONLY);

  
  
  pDevices = (deviceInfo *)malloc(sizeof(deviceInfo) * numDevices);

  hret = pDirectInput->CreateDevice(GUID_SysKeyboard,&pDevices[0].device,NULL);
  pDevices[0].isPolled = FALSE;
  pDevices[0].needed  = TRUE;

  if(hret != DI_OK) {
    //    errorMessage(myLoadString(IDS_ERROR_DISP_CREATEDEVICE), hret);
    return FALSE;
  }

  
  numDevices = 1;

  hret = pDirectInput->EnumDevices(DIDEVTYPE_JOYSTICK,
                                   DIEnumDevicesCallback,
                                   NULL,
                                   DIEDFL_ATTACHEDONLY);  

  //  hret = pDevices[0].device->SetCooperativeLevel(hWindow,
  //                                             DISCL_FOREGROUND|
  //                                             DISCL_NONEXCLUSIVE);
  
  if(hret != DI_OK) {
    //    errorMessage(myLoadString(IDS_ERROR_DISP_LEVEL), hret);
    return FALSE;
  }
  
  hret = pDevices[0].device->SetDataFormat(&c_dfDIKeyboard);

  if(hret != DI_OK) {
    //    errorMessage(myLoadString(IDS_ERROR_DISP_DATAFORMAT), hret);
    return FALSE;
  }

  for(int i = 1; i < numDevices; i++) {
    pDevices[i].device->SetDataFormat(&c_dfDIJoystick);
    pDevices[i].needed = FALSE;
    currentDevice = &pDevices[i];
    axisNumber = 0;
    currentDevice->device->EnumObjects(EnumAxesCallback, NULL, DIDFT_AXIS);
    currentDevice->device->EnumObjects(EnumPovsCallback, NULL, DIDFT_POV);
    /*
    if(joydebug) {
      // don't translate. debug only
      log("*************************\n");
      log("Joystick buttons : %d\n",    currentDevice->nButtons);
      log("Joystick center X: %08lx\n", currentDevice->centerX);
      log("Joystick center Y: %08lx\n", currentDevice->centerY);
      log("Joystick left    : %08lx\n", currentDevice->thresholdLeft);
      log("Joystick right   : %08lx\n", currentDevice->thresholdRight);
      log("Joystick up      : %08lx\n", currentDevice->thresholdUp);
      log("Joystick down    : %08lx\n", currentDevice->thresholdDown);
      log("Joystick polled  : %d\n",    currentDevice->isPolled);
    }
    */
    
    currentDevice = NULL;
  }

  for(i = 0; i < numDevices; i++)
    pDevices[i].device->Acquire();
  
  return TRUE;
}

BOOL checkKey(int key)
{
  int dev = (key >> 8);

  int k = (key & 255);
  
  if(dev == 0) {
    return KEYDOWN(pDevices[0].data,k);
  } else {
    if(k < 16) {
      int axis = k >> 1;
      LONG value = pDevices[dev].axis[axis].center;
      switch(pDevices[dev].axis[axis].offset) {
      case DIJOFS_X:
        value = pDevices[dev].state.lX;
        break;
      case DIJOFS_Y:
        value = pDevices[dev].state.lY;
        break;
      case DIJOFS_Z:
        value = pDevices[dev].state.lZ;
        break;
      case DIJOFS_RX:
        value = pDevices[dev].state.lRx;
        break;
      case DIJOFS_RY:
        value = pDevices[dev].state.lRy;
        break;
      case DIJOFS_RZ:
        value = pDevices[dev].state.lRz;
        break;
      case DIJOFS_SLIDER(0):
        value = pDevices[dev].state.rglSlider[0];
        break;
      case DIJOFS_SLIDER(1):
        value = pDevices[dev].state.rglSlider[1];
        break;
      }

      if(k & 1)
        return value > pDevices[dev].axis[axis].positive;
      return value < pDevices[dev].axis[axis].negative;
    } else if(k < 48) {
      int hat = (k >> 2) & 3;
      int state = getPovState(pDevices[dev].state.rgdwPOV[hat]);
      BOOL res = FALSE;
      switch(k & 3) {
      case 0:
        res = state & POV_UP;
        break;
      case 1:
        res = state & POV_DOWN;
        break;
      case 2:
        res = state & POV_RIGHT;
        break;
      case 3:
        res = state & POV_LEFT;
        break;
      }
      return res;
    } else if(k  >= 128) {
      return pDevices[dev].state.rgbButtons[k-128] & 0x80;
    }
  }

  return FALSE;
}

u32 systemReadJoypad()
{
  BOOL ok = TRUE;
  for(int i = 0; i < numDevices; i++) {
    if(i) {
      ok = readJoystick(i);
    } else
      ok = readKeyboard();
    if(!ok)
      return 0;
  }

  u32 res = 0;

  if(checkKey(joypad[KEY_BUTTON_A])) {
    res |= 1;
  }
  if(checkKey(joypad[KEY_BUTTON_B])) {
    res |= 2;
  }
  if(checkKey(joypad[KEY_BUTTON_SELECT]))
    res |= 4;
  if(checkKey(joypad[KEY_BUTTON_START]))
    res |= 8;
  if(checkKey(joypad[KEY_RIGHT]))
    res |= 16;
  if(checkKey(joypad[KEY_LEFT]))
    res |= 32;
  if(checkKey(joypad[KEY_UP]))
    res |= 64;
  if(checkKey(joypad[KEY_DOWN]))
    res |= 128;
  if(checkKey(joypad[KEY_BUTTON_R]))
    res |= 256;
  if(checkKey(joypad[KEY_BUTTON_L]))
    res |= 512;

  if(autoFire) {
    res &= (~autoFire);
    if(autoFireToggle)
      res |= autoFire;
    autoFireToggle = !autoFireToggle;
  }
  
  return res;
}

u32 systemReadJoypadExtended()
{
  u32 res = 0;

  if(checkKey(joypad[KEY_BUTTON_SPEED]) || speedupToggle)
    res |= 1;
  if(checkKey(joypad[KEY_BUTTON_CAPTURE]))
    res |= 2;
  if(checkKey(joypad[KEY_BUTTON_GS]))
    res |= 4;
  return res;
}

void systemDrawScreen()
{
  HRESULT hret;

  if(pDirectDraw == NULL ||
     ddsOffscreen == NULL ||
     ddsPrimary == NULL)
    return;

  for(std::list<IUpdateListener *>::iterator it = updateList.begin();
      it != updateList.end(); ) {
    IUpdateListener *up = *it;
    it++;
    up->update();
  }
  
  if(vsync && !speedup) {
    hret = pDirectDraw->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
  }
  
  DDSURFACEDESC2 ddsDesc;
  
  ZeroMemory(&ddsDesc, sizeof(ddsDesc));
  


  ddsDesc.dwSize = sizeof(ddsDesc);
  
  hret = ddsOffscreen->Lock(NULL,
                            &ddsDesc,
                            DDLOCK_WRITEONLY|
#ifndef FINAL_VERSION
                            DDLOCK_NOSYSLOCK|
#endif
                            DDLOCK_SURFACEMEMORYPTR,
                            NULL);

  if(hret == DDERR_SURFACELOST) {
    hret = ddsPrimary->Restore();
    if(hret == DD_OK) {
      hret = ddsOffscreen->Restore();
      
      if(hret == DD_OK) {
        hret = ddsOffscreen->Lock(NULL,
                                  &ddsDesc,
                                  DDLOCK_WRITEONLY|
#ifndef FINAL_VERSION
                                  DDLOCK_NOSYSLOCK|
#endif
                                  DDLOCK_SURFACEMEMORYPTR,
                                  NULL);
        
      }
    }
  }
    
  if(hret == DD_OK) {
    if(filterFunction) {
      (*filterFunction)(pix+filterWidth*2+2,
                        filterWidth*2+2,
                        (u8*)delta,
                        (u8*)ddsDesc.lpSurface,
                        ddsDesc.lPitch,
                        filterWidth,
                        filterHeight);
    } else {
      if(cartridgeType == 0) {
        __asm {
          push esi;
          push edi;
          push ecx;
          push edx;
          
          mov esi, pix;
          mov edi, ddsDesc.lpSurface;
          mov edx, ddsDesc.lPitch;
          mov ecx, 160;
          cmp systemColorDepth, 16;
          jnz gbaOtherColor;
          sub edx,480;
          add esi,482;
        gbaLoop16bit:
          push ecx;
          mov ecx, 240;
          repz movsw;
          inc esi;
          inc esi;
          add edi, edx;
          pop ecx;
          dec ecx;
          jnz gbaLoop16bit;
          jmp gbaLoopEnd;
        gbaOtherColor:
          cmp systemColorDepth, 24;
          jnz gbaOtherColor2;
          sub edx, 240*3;
        gbaLoop24bit:
          push ecx;
          mov ecx, 240*3;
          repz movsb;
          add edi, edx;
          pop ecx;
          dec ecx;
          jnz gbaLoop24bit;
          jmp gbaLoopEnd;
        gbaOtherColor2:
          sub edx, 240*4;
        gbaLoop32bit:
          push ecx;
          mov ecx, 240;
          repz movsd;
          add edi, edx;
          pop ecx;
          dec ecx;
          jnz gbaLoop32bit;
        gbaLoopEnd:
          pop edx;
          pop ecx;
          pop edi;
          pop esi;
        }
      } else {
        if(gbBorderOn) {
          __asm {
            push esi;
            push edi;
            push ecx;
            push edx;
            
            mov esi, pix;
            mov edi, ddsDesc.lpSurface;
            mov edx, ddsDesc.lPitch;
            mov ecx, 224;
            cmp systemColorDepth, 16;
            jnz otherColor;
            add esi, 514;
          loop123: 
            push ecx;
            
            mov ecx, 256;
            repz movsw;
            inc esi;
            inc esi;
            add edi, edx;
            sub edi, 256 * 2;
            
            pop ecx;
            dec ecx;
            jnz loop123;
            jmp loopEnd;
          otherColor:
            cmp systemColorDepth, 24;
            jnz otherColor2;
          loop456: 
            push ecx;
            
            mov ecx, 256 * 3;
            repz movsb;
            
            add edi, edx;
            sub edi, 256 * 3;
            
            pop ecx;
            dec ecx;
            jnz loop456;
            jmp loopEnd;
          otherColor2:
            push ecx;
            mov ecx, 256;
            repz movsd;
            add edi, edx;
            sub edi, 256 * 4;
            
            pop ecx;
            dec ecx;
            jnz otherColor2;
          loopEnd:
            pop edx;
            pop ecx;
            pop edi;
            pop esi;
          }
        } else {
          __asm {
            push esi;
            push edi;
            push ecx;
            push edx;
            
            mov esi, pix;
            mov edi, ddsDesc.lpSurface;
            mov edx, ddsDesc.lPitch;
            mov ecx, 144;
            cmp systemColorDepth, 16;
            jnz otherColor3;
            add esi, 322;
          loop1234: 
            push ecx;
            
            mov ecx, 160;
            repz movsw;
            inc esi;
            inc esi;
            add edi, edx;
            sub edi, 160 * 2;
            
            pop ecx;
            dec ecx;
            jnz loop1234;
            jmp loopEnd2;
          otherColor3:
            cmp systemColorDepth, 24;
            jnz otherColor4;
          loop4567: 
            push ecx;
            
            mov ecx, 160 * 3;
            repz movsb;
            
            add edi, edx;
            sub edi, 160 * 3;
            
            pop ecx;
            dec ecx;
            jnz loop4567;
            jmp loopEnd2;
          otherColor4:
            push ecx;
            mov ecx, 160;
            repz movsd;
            add edi, edx;
            sub edi, 160 * 4;
            
            pop ecx;
            dec ecx;
            jnz otherColor4;
          loopEnd2:
            pop edx;
            pop ecx;
            pop edi;
            pop esi;
          }
        }      
      }
    }
  }
  hret = ddsOffscreen->Unlock(NULL);
  
  if(hret == DD_OK) {
    hret = ddsPrimary->Blt(&dest, ddsOffscreen, &rect,DDBLT_ASYNC,NULL);
    
    if(hret == DDERR_SURFACELOST) {
      hret = ddsPrimary->Restore();
      
      if(hret == DD_OK) {
        hret = ddsPrimary->Blt(&dest, ddsOffscreen, &rect, DDBLT_ASYNC, NULL);
      }
    }
  }

  if(screenMessage) {
    if(((GetTickCount() - screenMessageTime) < 3000) &&
       !disableStatusMessage) {
      HDC hdc;
      ddsPrimary->GetDC(&hdc);
      SetTextColor(hdc, RGB(255,0,0));
      SetBkMode(hdc,TRANSPARENT);      
      TextOut(hdc, dest.left+10, dest.bottom - 20, screenMessageBuffer,
              strlen(screenMessageBuffer));
      ddsPrimary->ReleaseDC(hdc);
    } else {
      screenMessage = false;
    }
  }
  
  if(hret != DD_OK) {
    //    if(drawdebug) {
      // don't translate. Debug only
    //      log("Error on update screen: %08xl\n", hret);
    //    }
  }
}

int ffs (UINT mask)
{
  int m = 0;
  if (mask) {
    while (!(mask & (1 << m)))
      m++;
    
    return (m);
  }
  
  return (0);
}

void adjustDestRect()
{
  POINT point;

  point.x = 0;
  point.y = 0;

  if(ClientToScreen(hWindow, &point)) {
    dest.top = point.y;
    dest.left = point.x;
  }

  point.x = surfaceSizeX;
  point.y = surfaceSizeY;

  if(ClientToScreen(hWindow, &point)) {
    dest.bottom = point.y;
    dest.right = point.x;
  }

  int menuSkip = 0;
  
  if(videoOption >= VIDEO_320x240 && menuToggle) {
    int m = GetSystemMetrics(SM_CYMENU);
    menuSkip = m;
    dest.bottom -=m;
  }

  if(videoOption == VIDEO_320x240) {
    if(cartridgeType == 0) {
      dest.top += 40;
      dest.bottom += 40;
      dest.left += 40;
      dest.right += 40;
    } else {
      if(gbBorderOn) {
        dest.top += 8;
        dest.bottom += 8;
        dest.left += 32;
        dest.right += 32;
      } else {
        dest.top += 48;
        dest.bottom += 48;
        dest.left += 80;
        dest.right += 80;
      }
    }
    if(fullScreenStretch) {
      dest.top = 0+menuSkip;
      dest.left = 0;
      dest.right = 320;
      dest.bottom = 240;
    }
  }

  if(videoOption == VIDEO_640x480) {
    if(cartridgeType == 0) {
      dest.top += 80;
      dest.bottom += 80;
      dest.left += 80;
      dest.right += 80;
    } else {
      if(gbBorderOn) {
        dest.top += 16;
        dest.bottom += 16;
        dest.left += 64;
        dest.right += 64;
      } else {
        dest.top += 24;
        dest.bottom += 24;
        dest.left += 80;
        dest.right += 80;
      }
    }
    if(fullScreenStretch) {
      dest.top = 0+menuSkip;
      dest.left = 0;
      dest.right = 640;
      dest.bottom = 480;
    }          
  }
  if(videoOption == VIDEO_800x600) {
    if(cartridgeType == 0) {
      dest.top += 60;
      dest.bottom += 60;
      dest.left += 40;
      dest.right += 40;
    } else {
      if(gbBorderOn) {
        dest.top += 76;
        dest.bottom += 76;
        dest.left += 144;
        dest.right += 144;
      } else {
        dest.top += 12;
        dest.bottom += 12;
        dest.left += 80;
        dest.right += 80;
      }
    }
    if(fullScreenStretch) {
      dest.top = 0+menuSkip;
      dest.left = 0;
      dest.right = 800;
      dest.bottom = 600;
    }          
  }  
}

void updateFilter()
{
  if(systemColorDepth == 16 && (videoOption > VIDEO_1X &&
                          videoOption != VIDEO_320x240)) {
    switch(filterType) {
    default:
    case 0:
      filterFunction = NULL;
      break;
    case 1:
      filterFunction = TVMode;
      break;
    case 2:
      filterFunction = _2xSaI;
      break;
    case 3:
      filterFunction = Super2xSaI;
      break;
    case 4:
      filterFunction = SuperEagle;
      break;
    case 5:
      filterFunction = Pixelate;
      break;
    case 6:
      filterFunction = MotionBlur;
      break;
    case 7:
      filterFunction = AdMame2x;
      break;
    case 8:
      filterFunction = Simple2x;
      break;
    }
    if(filterType != 0) {
      filterWidth = sizeX;
      filterHeight = sizeY;
      rect.right = sizeX*2;
      rect.bottom = sizeY*2;
      memset(delta,255,sizeof(delta));
    } else {
      rect.right = sizeX;
      rect.bottom = sizeY;
    }
  } else {
    filterFunction = NULL;
  }
}

typedef BOOL (WINAPI *GETMENUBARINFO)(HWND, LONG, LONG, PMENUBARINFO);

void winCheckMenuBarInfo(int& winSizeX, int& winSizeY)
{
  HINSTANCE hinstDll;
  DWORD dwVersion = 0;
  
  hinstDll = LoadLibrary("USER32.DLL");
  
  if(hinstDll) {
    GETMENUBARINFO func = (GETMENUBARINFO)GetProcAddress(hinstDll,
                                                         "GetMenuBarInfo");

    if(func) {
      MENUBARINFO info;
      info.cbSize = sizeof(info);
      
      func(hWindow, OBJID_MENU, 0, &info);
      
      int menuHeight = GetSystemMetrics(SM_CYMENU);
      
      if((info.rcBar.bottom - info.rcBar.top) > menuHeight) {
        winSizeY += (info.rcBar.bottom - info.rcBar.top) - menuHeight + 1;
        SetWindowPos(hWindow,
                     0, //HWND_TOPMOST,
                     windowPositionX,
                     windowPositionY,
                     winSizeX,
                     winSizeY,
                     SWP_NOMOVE | SWP_SHOWWINDOW);
      }
    }
    FreeLibrary(hinstDll);
  }
}

void updateWindowSize(int value)
{
  if(value != videoOption) {
    regSetDwordValue("video", value);
  }
  
  if(((value >= VIDEO_320x240) &&
     (videoOption != value)) ||
     (videoOption >= VIDEO_320x240 &&
      value <= VIDEO_4X)) {
    changingVideoSize = TRUE;
    shutdownDirectDraw();
    shutdownDirectInput();
    DragAcceptFiles(hWindow, FALSE);
    DestroyWindow(hWindow);
    hWindow = NULL;
    videoOption = value;
    if(!initDirectDraw()) {
      if(videoOption == VIDEO_320x240 ||
         videoOption == VIDEO_640x480 ||
         videoOption == VIDEO_800x600)
        regSetDwordValue("video", VIDEO_1X);
      changingVideoSize = FALSE;
      fileExit();
      return;
    }
    if(!initDirectInput()) {
      changingVideoSize = FALSE;
      fileExit();
      return;
    }
    checkKeys();
    updateMenuBar();
    changingVideoSize = FALSE;
    updateWindowSize(videoOption);
    return;
  }
  
  sizeX = 240;
  sizeY = 160;

  videoOption = value;
  
  if(cartridgeType == 1) {
    if(gbBorderOn) {
      sizeX = 256;
      sizeY = 224;
      gbBorderLineSkip = 256;
      gbBorderColumnSkip = 48;
      gbBorderRowSkip = 40;
    } else {
      sizeX = 160;
      sizeY = 144;
      gbBorderLineSkip = 160;
      gbBorderColumnSkip = 0;
      gbBorderRowSkip = 0;
    }
  }
  
  surfaceSizeX = sizeX;
  surfaceSizeY = sizeY;
  
  switch(videoOption) {
  case VIDEO_1X:
    surfaceSizeX = sizeX;
    surfaceSizeY = sizeY;
    break;
  case VIDEO_2X:
    surfaceSizeX = sizeX * 2;
    surfaceSizeY = sizeY * 2;
    break;
  case VIDEO_3X:
    surfaceSizeX = sizeX * 3;
    surfaceSizeY = sizeY * 3;
    break;
  case VIDEO_4X:
    surfaceSizeX = sizeX * 4;
    surfaceSizeY = sizeY * 4;
    break;
  case VIDEO_320x240:
    surfaceSizeX = sizeX;
    surfaceSizeY = sizeY;
    if(fullScreenStretch) {
      surfaceSizeX = 320;
      surfaceSizeY = 240;
    }
    break;
  case VIDEO_640x480:
    if(cartridgeType == 0) {
      surfaceSizeX = sizeX * 2;
      surfaceSizeY = sizeY * 2;
    } else {
      if(gbBorderOn) {
        surfaceSizeX = sizeX * 2;
        surfaceSizeY = sizeY * 2;
      } else {
        surfaceSizeX = sizeX * 3;
        surfaceSizeY = sizeY * 3;
      }      
    }
    if(fullScreenStretch) {
      surfaceSizeX = 640;
      surfaceSizeY = 480;
    }
    break;
  case VIDEO_800x600:
    if(cartridgeType == 0) {
      surfaceSizeX = sizeX * 3;
      surfaceSizeY = sizeY * 3;
    } else {
      if(gbBorderOn) {
        surfaceSizeX = sizeX * 2;
        surfaceSizeY = sizeY * 2;
      } else {
        surfaceSizeX = sizeX * 4;
        surfaceSizeY = sizeY * 4;
      }      
    }
    if(fullScreenStretch) {
      surfaceSizeX = 800;
      surfaceSizeY = 600;
    }
    break;
  }

  rect.right = sizeX;
  rect.bottom = sizeY;

  int winSizeX = sizeX;
  int winSizeY = sizeY;
  
  if(videoOption <= VIDEO_4X) {
    dest.left = 0;
    dest.top = 0;
    dest.right = surfaceSizeX;
    dest.bottom = surfaceSizeY;    
    
    DWORD style = WS_POPUP | WS_VISIBLE;
    
    style |= WS_OVERLAPPEDWINDOW;
    
    menuToggle = TRUE;
    AdjustWindowRectEx(&dest, style, TRUE, 0); //WS_EX_TOPMOST);
    
    winSizeX = dest.right-dest.left;
    winSizeY = dest.bottom-dest.top;
    
    SetWindowPos(hWindow,
                 0, //HWND_TOPMOST,
                 windowPositionX,
                 windowPositionY,
                 winSizeX,
                 winSizeY,
                 SWP_NOMOVE | SWP_SHOWWINDOW);

    winCheckMenuBarInfo(winSizeX, winSizeY);
  }

  adjustDestRect();

  updateFilter();
  
  RedrawWindow(hWindow,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);  
}

void updateVideoSize(UINT id)
{
  int value = 0;

  switch(id) {
  case ID_OPTIONS_VIDEO_X1:
    value = VIDEO_1X;
    break;
  case ID_OPTIONS_VIDEO_X2:
    value = VIDEO_2X;
    break;
  case ID_OPTIONS_VIDEO_X3:
    value = VIDEO_3X;
    break;
  case ID_OPTIONS_VIDEO_X4:
    value = VIDEO_4X;
    break;
  case ID_OPTIONS_VIDEO_FULLSCREEN320X240:
    value = VIDEO_320x240;
    break;
  case ID_OPTIONS_VIDEO_FULLSCREEN640X480:
    value = VIDEO_640x480;
    break;
  case ID_OPTIONS_VIDEO_FULLSCREEN800X600:
    value = VIDEO_800x600;
    break;
  }

  if(videoOption == value)
    return;

  updateWindowSize(value);
}

char *winLoadFilter(int id)
{
  char *p = (char *)winResLoadString(id);
  char *res = p;

  while(*p) {
    if(*p == '_')
      *p = 0;
    p++;
  }
  
  return res;
}

BOOL emulatorSelectBiosFile()
{
  OPENFILENAME ofn;

  szFile[0] = 0;
  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWindow;
  ofn.lpstrFile = biosFileName;
  ofn.nMaxFile = sizeof(biosFileName);
  ofn.lpstrFilter =  winLoadFilter(IDS_FILTER_BIOS);
  ofn.nFilterIndex = 0; //selectedFileIndex;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = winResLoadString(IDS_SELECT_BIOS_FILE); //szSelectRom;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
  
  if(videoOption == VIDEO_320x240) {
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG);
    ofn.Flags |= OFN_ENABLETEMPLATE;
  }
  
  if(GetOpenFileName(&ofn) == FALSE) {
    DWORD res = CommDlgExtendedError();
    return FALSE;
  }
  
  return TRUE;
}

extern void emulatorAssociate();

void fileClose()
{
  // save battery file before we change the filename...
  if(rom != NULL || gbRom != NULL) {
    if(autoSaveLoadCheatList)
      winSaveCheatListDefault();
    writeBatteryFile();
    emuCleanUp();
    remoteCleanUp();
  }
  emulating = 0;
  RedrawWindow(hWindow,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
  systemSetTitle("VisualBoyAdvance");

  setScreenSaverEnable(screenSaverState);
}

void fileImportGSSnapshot()
{
  OPENFILENAME ofn;

  szFile[0] = 0;
  
  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWindow;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  if(cartridgeType == 1)
    ofn.lpstrFilter = winLoadFilter(IDS_FILTER_GBS);
  else
    ofn.lpstrFilter = winLoadFilter(IDS_FILTER_SPS);
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = winResLoadString(IDS_SELECT_SNAPSHOT_FILE);
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if(videoOption == VIDEO_320x240) {
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG);
    ofn.Flags |= OFN_ENABLETEMPLATE;
  }  

  if(GetOpenFileName(&ofn) == FALSE) {
    DWORD res = CommDlgExtendedError();
    return;
  }

  if(MessageBox(hWindow,
                winResLoadString(IDS_SAVE_WILL_BE_LOST),
                winResLoadString(IDS_CONFIRM_ACTION),
                MB_OKCANCEL) == IDCANCEL)
    return;
  
  bool res = false;
  
  if(cartridgeType == 1)
    res = gbReadGSASnapshot(ofn.lpstrFile);
  else
    res = CPUReadGSASnapshot(ofn.lpstrFile);
}

void fileImportEepromFile()
{
  OPENFILENAME ofn;

  szFile[0] = 0;
  
  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWindow;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = winLoadFilter(IDS_FILTER_SAV);
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = winResLoadString(IDS_SELECT_BATTERY_FILE);
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if(videoOption ==  VIDEO_320x240) {
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG);
    ofn.Flags |= OFN_ENABLETEMPLATE;
  }
  
  if(GetOpenFileName(&ofn) == FALSE) {
    DWORD res = CommDlgExtendedError();
    return;
  }

  if(MessageBox(hWindow,
                winResLoadString(IDS_SAVE_WILL_BE_LOST),
                winResLoadString(IDS_CONFIRM_ACTION),
                MB_OKCANCEL) == IDCANCEL)
    return;
  
  bool res = false;
  
  res = CPUImportEepromFile(ofn.lpstrFile);

  if(!res)
    systemMessage(MSG_CANNOT_OPEN_FILE, "Cannot open file %s", ofn.lpstrFile);
  else {
    CPUReset();
  }
}

void fileImportBatteryFile()
{
  OPENFILENAME ofn;

  szFile[0] = 0;
  
  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWindow;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = winLoadFilter(IDS_FILTER_SAV);
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = winResLoadString(IDS_SELECT_BATTERY_FILE);
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if(videoOption == VIDEO_320x240) {
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG);
    ofn.Flags |= OFN_ENABLETEMPLATE;
  }
  
  if(GetOpenFileName(&ofn) == FALSE) {
    DWORD res = CommDlgExtendedError();
    return;
  }

  if(MessageBox(hWindow,
                winResLoadString(IDS_SAVE_WILL_BE_LOST),
                winResLoadString(IDS_CONFIRM_ACTION),
                MB_OKCANCEL) == IDCANCEL)
    return;
  
  bool res = false;

  res = emuReadBattery(ofn.lpstrFile);
  
  if(!res)
    systemMessage(MSG_CANNOT_OPEN_FILE, "Cannot open file %s", ofn.lpstrFile);
  else {
    emuReset();
  }
}

bool fileImportGSACodeFile(char *fileName)
{
  FILE *f = fopen(fileName, "rb");
  
  if(f == NULL) {
    systemMessage(MSG_CANNOT_OPEN_FILE, "Cannot open file %s", fileName);
    return false;
  }

  u32 len;
  fread(&len, 1, 4, f);
  if(len != 14) {
    fclose(f);
    systemMessage(MSG_UNSUPPORTED_CODE_FILE, "Unsupported code file %s",
                  fileName);
    return false;
  }
  char buffer[16];
  fread(buffer, 1, 14, f);
  buffer[14] = 0;
  if(memcmp(buffer, "SharkPortCODES", 14)) {
    fclose(f);
    systemMessage(MSG_UNSUPPORTED_CODE_FILE, "Unsupported code file %s",
                  fileName);
    return false;
  }    
  fseek(f, 0x1e, SEEK_SET);
  fread(&len, 1, 4, f);
  int game = 0;
  if(len > 1) {
    game = winGSACodeSelect(hWindow,
                            (LPARAM)f);
  }
  fclose(f);

  if(game != -1) {
    return cheatsImportGSACodeFile(fileName, game);
  }
  
  return true;
}

void fileImportGSCodeFile()
{
  OPENFILENAME ofn;

  szFile[0] = 0;
  
  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWindow;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  if(cartridgeType == 0)
    ofn.lpstrFilter = winLoadFilter(IDS_FILTER_SPC);
  else
    ofn.lpstrFilter = winLoadFilter(IDS_FILTER_GCF);
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = winResLoadString(IDS_SELECT_CODE_FILE);
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if(videoOption == VIDEO_320x240) {
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG);
    ofn.Flags |= OFN_ENABLETEMPLATE;
  }
  
  if(GetOpenFileName(&ofn) == FALSE) {
    DWORD res = CommDlgExtendedError();
    return;
  }

  if(MessageBox(hWindow,
                winResLoadString(IDS_CODES_WILL_BE_LOST),
                winResLoadString(IDS_CONFIRM_ACTION),
                MB_OKCANCEL) == IDCANCEL)
    return;
  
  bool res = false;
  
  if(cartridgeType == 1)
    res = gbCheatReadGSCodeFile(ofn.lpstrFile);
  else {
    res = fileImportGSACodeFile(ofn.lpstrFile);
  }
}

void fileExportEepromFile()
{
  OPENFILENAME ofn;

  char *p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  sprintf(szFile, "%s", p);
  
  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWindow;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = winLoadFilter(IDS_FILTER_SAV);
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrDefExt = "SAV";
  ofn.lpstrTitle = winResLoadString(IDS_SELECT_BATTERY_FILE);
  ofn.Flags = OFN_PATHMUSTEXIST;

  if(videoOption == VIDEO_320x240) {
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG);
    ofn.Flags |= OFN_ENABLETEMPLATE;
  }
  
  if(GetSaveFileName(&ofn) == FALSE) {
    DWORD res = CommDlgExtendedError();
    return;
  }

  bool res = false;
  
  res = CPUExportEepromFile(ofn.lpstrFile);

  if(!res)
    systemMessage(MSG_ERROR_CREATING_FILE, "Error creating file %s",
                  ofn.lpstrFile);
}

void fileExportBatteryFile()
{
  char *p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  sprintf(szFile, "%s", p);

  char *exts[] = {".sav", ".dat" };

  FileDlg dlg(hWindow,
              (char *)szFile,
              (int)sizeof(szFile),
              (char *)winLoadFilter(IDS_FILTER_SAV),
              1,
              "SAV",
              exts,
              (char *)NULL, 
              (char *)winResLoadString(IDS_SELECT_BATTERY_FILE),
              TRUE);

  BOOL res = dlg.DoModal();  
  if(res == FALSE) {
    DWORD res = CommDlgExtendedError();
    return;
  }

  bool result = false;

  if(cartridgeType == 1) {
    result = gbWriteBatteryFile(szFile, false);
  } else
    result = emuWriteBattery(szFile);

  if(!result)
    systemMessage(MSG_ERROR_CREATING_FILE, "Error creating file %s",
                  szFile);
}

void fileExportGSASnapshot()
{
  if(eepromInUse) {
    systemMessage(IDS_EEPROM_NOT_SUPPORTED, "EEPROM saves cannot be exported");
    return;
  }
  
  char *p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  sprintf(szFile, "%s", p);

  char *exts[] = {".sps" };

  FileDlg dlg(hWindow,
              (char *)szFile,
              (int)sizeof(szFile),
              (char *)winLoadFilter(IDS_FILTER_SPS),
              1,
              "SPS",
              exts,
              (char *)NULL, 
              (char *)winResLoadString(IDS_SELECT_SNAPSHOT_FILE),
              TRUE);

  BOOL res = dlg.DoModal();  
  if(res == FALSE) {
    DWORD res = CommDlgExtendedError();
    return;
  }

  fileExportSPSSnapshot(szFile, p);
}

extern void helpAbout();
extern void toolsLogging();
extern void toolsDisassemble();
extern void toolsMapView();
extern void toolsMemoryViewer();
extern void toolsPaletteView();
extern void toolsOamViewer();
extern void toolsTileViewer();
extern void toolsCustomize();
extern void toolsDebugGDB();
extern void toolsDebugGDBLoad();
extern void optionsGameboyColors();

//-----------------------------------------------------------------------------
// Name: WindowProc()
// Desc: The Main Window Procedure
//-----------------------------------------------------------------------------
long FAR PASCAL
WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {
  case WM_ACTIVATE:
    {
      BOOL a = (wParam == WA_ACTIVE) || (wParam == WA_CLICKACTIVE);
      if(a && pDevices) {
        active = a;
        // Pause if minimized or not the top window
        for(int i = 0; i < numDevices; i++) {
          if(pDevices != NULL && pDevices[i].device != NULL)
            pDevices[i].device->Acquire();
        }
        if(!paused) {
          if(emulating)
            soundResume();
        }
      } else {
        if(pauseWhenInactive) {
          if(emulating)
            soundPause();
          active = a;
        }
      }
    }
    if(paused && emulating)
      systemDrawScreen();
    break;
  case WM_DESTROY:
    if(!changingVideoSize) {
      PostQuitMessage(0);
      emulating = 0;
    }
    return 0L;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      BeginPaint(hWnd, &ps);
      if(emulating)
        systemDrawScreen();
      EndPaint(hWnd, &ps);
    }
    return TRUE;
  case WM_MOVE:
    {
      if(!changingVideoSize) {
        if(hWindow) {
          if(!IsIconic(hWindow)) {
            RECT r;
            
            GetWindowRect(hWindow,&r);
            windowPositionX = r.left;
            windowPositionY = r.top;
            adjustDestRect();
            regSetDwordValue("windowX", windowPositionX);
            regSetDwordValue("windowY", windowPositionY);
          }
        }
      }
      return 0L;        
    }
  case WM_SIZE:
    {
      if(!changingVideoSize) {
        if(hWindow) {
          if(!IsIconic(hWindow)) {
            if(iconic) {
              if(emulating) {
                soundResume();
                paused = FALSE;
              }
            }
            if(videoOption <= VIDEO_4X) {
              surfaceSizeX = LOWORD(lParam);
              surfaceSizeY = HIWORD(lParam);
              adjustDestRect();
            }
          } else {
            if(emulating) {
              if(!paused) {
                paused = TRUE;
                soundPause();
              }
            }
            iconic = TRUE;                  
          }
        }
      }
    }
    break;
  case WM_DROPFILES:
    {
      HDROP hDrop = (HDROP)wParam;
      if(DragQueryFile(hDrop,
                       0,
                       szFile,
                       1024)) {
        if(fileOpen()) {
          SetForegroundWindow(hWindow);
          emulating = TRUE;
        } else {
          emulating = FALSE;
          soundPause();
        }
      }
      DragFinish(hDrop);
    }
    return 0L;
  case WM_INITMENU:
    soundPause();
    break;
  case WM_INITMENUPOPUP:
    {
      HMENU menu = (HMENU)wParam;
      int menuNum = LOWORD(lParam);
      int top = HIWORD(lParam);

      if(!top) {
        switch(menuNum) {
        case MENU_FILE:
          updateFileMenu(menu);
          break;
        case MENU_OPTIONS:
          updateFrameSkipMenu(menu);
          updateVideoMenu(menu);
          updateEmulatorMenu(menu);
          updateSoundMenu(menu);
          updateGameboyMenu(menu);
          updatePriorityMenu(menu);
          updateFilterMenu(menu);
          updateJoypadMenu(menu);
          updateLanguageMenu(menu);
          break;
        case MENU_CHEATS:
          updateCheatsMenu(menu);
          break;
        case MENU_TOOLS:
          updateToolsMenu(menu);
          break;
        }
      }
    }
    break;
  case WM_KEYDOWN:
    // Handle any non-accelerated key commands
    switch (wParam) {
    case VK_ESCAPE:
      fileToggleMenu();
      return 0L;
    }
    break;
  case WM_MOUSEMOVE:
    winMouseOn();
    break;
  case WM_LBUTTONDOWN:
    winMouseOn();
    break;
  case WM_LBUTTONUP:
    winMouseOn();
    break;
  case WM_RBUTTONDOWN:
    winMouseOn();
    break;
  case WM_RBUTTONUP:
    winMouseOn();
    break;
  case WM_MBUTTONDOWN:
    winMouseOn();
    break;
  case WM_MBUTTONUP:
    winMouseOn();
    break;
  case WM_COMMAND:
    switch(wParam & 0xFFFF) {
    case ID_FILE_OPEN:
      if(fileOpenSelect()) {
        if(fileOpen())
          emulating = TRUE;
        else {
          emulating = FALSE;
          soundPause();
        }
      }
      break;
    case ID_FILE_OPENGAMEBOY:
      if(fileOpenSelectGB()) {
        if(fileOpen())
          emulating = TRUE;
        else {
          emulating = FALSE;
          soundPause();
        }
      }
      break;
    case ID_FILE_LOAD:
      if(emulating)
        loadSaveGame();
      break;
    case ID_FILE_SAVE:
      if(emulating)
        writeSaveGame();
      break;
    case ID_FILE_LOADGAME_SLOT1:
    case ID_FILE_LOADGAME_SLOT2:
    case ID_FILE_LOADGAME_SLOT3:
    case ID_FILE_LOADGAME_SLOT4:
    case ID_FILE_LOADGAME_SLOT5:
    case ID_FILE_LOADGAME_SLOT6:
    case ID_FILE_LOADGAME_SLOT7:
    case ID_FILE_LOADGAME_SLOT8:
    case ID_FILE_LOADGAME_SLOT9:
    case ID_FILE_LOADGAME_SLOT10:
      if(emulating)
        loadSaveGame((wParam&0xffff)+1-ID_FILE_LOADGAME_SLOT1);
      break;
    case ID_FILE_SAVEGAME_SLOT1:
    case ID_FILE_SAVEGAME_SLOT2:
    case ID_FILE_SAVEGAME_SLOT3:
    case ID_FILE_SAVEGAME_SLOT4:
    case ID_FILE_SAVEGAME_SLOT5:
    case ID_FILE_SAVEGAME_SLOT6:
    case ID_FILE_SAVEGAME_SLOT7:
    case ID_FILE_SAVEGAME_SLOT8:
    case ID_FILE_SAVEGAME_SLOT9:
    case ID_FILE_SAVEGAME_SLOT10:
      if(emulating)
        writeSaveGame((wParam&0xffff)+1-ID_FILE_SAVEGAME_SLOT1);
      break;
    case ID_FILE_CLOSE:
      if(emulating)
        fileClose();
      break;
    case ID_FILE_RECENT_RESET:
      resetRecentList();
      break;
    case ID_FILE_RECENT_FREEZE:
      recentFreeze = !recentFreeze;
      regSetDwordValue("recentFreeze", recentFreeze);
      break;
    case ID_FILE_MRU_FILE1:
    case ID_FILE_MRU_FILE2:
    case ID_FILE_MRU_FILE3:
    case ID_FILE_MRU_FILE4:
    case ID_FILE_MRU_FILE5:
    case ID_FILE_MRU_FILE6:
    case ID_FILE_MRU_FILE7:
    case ID_FILE_MRU_FILE8:
    case ID_FILE_MRU_FILE9:
    case ID_FILE_MRU_FILE10:
      if(recentFiles[(wParam&0xFFFF)-ID_FILE_MRU_FILE1]) {
        strcpy(szFile, recentFiles[(wParam&0xFFFF)-ID_FILE_MRU_FILE1]);
        if(fileOpen())
          emulating = TRUE;
        else {
          emulating = FALSE;
          soundPause();
        }
      }
      break;
    case ID_FILE_IMPORT_GAMESHARKCODEFILE:
      fileImportGSCodeFile();
      break;
    case ID_FILE_IMPORT_GAMESHARKSNAPSHOT:
      fileImportGSSnapshot();
      break;
    case ID_FILE_IMPORT_BATTERYFILE:
      fileImportBatteryFile();
      break;
    case ID_FILE_EXPORT_BATTERYFILE:
      fileExportBatteryFile();
      break;
    case ID_FILE_EXPORT_GAMESHARKSNAPSHOT:
      fileExportGSASnapshot();
      break;
    case ID_FILE_PAUSE:
      paused = !paused;
      if(paused) {
        soundPause();
        setScreenSaverEnable(screenSaverState);
      } else {
        soundResume();
        if(screenSaverState)
          setScreenSaverEnable(FALSE);
      }
      break;
    case ID_FILE_RESET:
      if(emulating) {
        emuReset();
        systemScreenMessage((char *)winResLoadString(IDS_RESET));
      }
      break;
    case ID_FILE_SCREENCAPTURE:
      screenCapture();
      break;
    case ID_FILE_ROMINFORMATION:
      if(cartridgeType == 0)
        winGBARomInfo(rom);
      else
        winGBRomInfo(gbRom);
      break;
    case ID_FILE_EXIT:
      fileExit();
      break;
    case ID_OPTIONS_VIDEO_FRAMESKIP_0:
    case ID_OPTIONS_VIDEO_FRAMESKIP_1:
    case ID_OPTIONS_VIDEO_FRAMESKIP_2:
    case ID_OPTIONS_VIDEO_FRAMESKIP_3:
    case ID_OPTIONS_VIDEO_FRAMESKIP_4:
    case ID_OPTIONS_VIDEO_FRAMESKIP_5:
      if(cartridgeType == 0) {
        frameSkip = (wParam&0xffff) - ID_OPTIONS_VIDEO_FRAMESKIP_0;
        regSetDwordValue("frameSkip", frameSkip);
      } else {
        gbFrameSkip = (wParam&0xffff) - ID_OPTIONS_VIDEO_FRAMESKIP_0;
        regSetDwordValue("gbFrameSkip", gbFrameSkip);   
      }
      break;
    case ID_OPTIONS_VIDEO_FRAMESKIP_6:
    case ID_OPTIONS_VIDEO_FRAMESKIP_7:
    case ID_OPTIONS_VIDEO_FRAMESKIP_8:
    case ID_OPTIONS_VIDEO_FRAMESKIP_9:
      if(cartridgeType == 0) {
        frameSkip = 6 + (wParam&0xffff) - ID_OPTIONS_VIDEO_FRAMESKIP_6;
        regSetDwordValue("frameSkip", frameSkip);
      } else {
        gbFrameSkip = 6 + (wParam&0xffff) - ID_OPTIONS_VIDEO_FRAMESKIP_6;
        regSetDwordValue("gbFrameSkip", gbFrameSkip);   
      }
      break;
    case ID_OPTIONS_VIDEO_VSYNC:
      vsync = !vsync;
      regSetDwordValue("vsync", vsync);
      break;
    case ID_OPTIONS_VIDEO_LAYERS_BG0:
    case ID_OPTIONS_VIDEO_LAYERS_BG1:
    case ID_OPTIONS_VIDEO_LAYERS_BG2:
    case ID_OPTIONS_VIDEO_LAYERS_BG3:
    case ID_OPTIONS_VIDEO_LAYERS_OBJ:
    case ID_OPTIONS_VIDEO_LAYERS_WIN0:
    case ID_OPTIONS_VIDEO_LAYERS_WIN1:
    case ID_OPTIONS_VIDEO_LAYERS_OBJWIN:
      layerSettings ^= 0x0100 <<
        ((wParam & 0xFFFF) - ID_OPTIONS_VIDEO_LAYERS_BG0);
      layerEnable = DISPCNT & layerSettings;
      break;
    case ID_OPTIONS_VIDEO_X1:
    case ID_OPTIONS_VIDEO_X2:
    case ID_OPTIONS_VIDEO_X3:
    case ID_OPTIONS_VIDEO_X4:
    case ID_OPTIONS_VIDEO_FULLSCREEN320X240:
    case ID_OPTIONS_VIDEO_FULLSCREEN640X480:
    case ID_OPTIONS_VIDEO_FULLSCREEN800X600:
      updateVideoSize(wParam&0xffff);
      break;
    case ID_OPTIONS_VIDEO_DISABLESFX:
      cpuDisableSfx = !cpuDisableSfx;
      regSetDwordValue("disableSfx", cpuDisableSfx);
      if(emulating && cartridgeType == 0)
        CPUUpdateRender();
      break;
    case ID_OPTIONS_VIDEO_FULLSCREENSTRETCHTOFIT:
      fullScreenStretch = !fullScreenStretch;
      regSetDwordValue("stretch", fullScreenStretch);
      updateWindowSize(videoOption);
      break;
    case ID_OPTIONS_VIDEO_DDRAWEMULATIONONLY:
      ddrawEmulationOnly = !ddrawEmulationOnly;
      regSetDwordValue("ddrawEmulationOnly", ddrawEmulationOnly);
      if(ddrawUseVideoMemory && ddrawEmulationOnly) {
        systemMessage(IDS_DISABLING_VIDEO_MEMORY,
                      "Disabling Use Video Memory setting");
        ddrawUseVideoMemory = false;
        regSetDwordValue("ddrawUseVideoMemory", ddrawUseVideoMemory);   
      }
      systemMessage(IDS_SETTING_WILL_BE_EFFECTIVE,
                    "Setting will be effective the next time you start the emulator");
      break;
    case ID_OPTIONS_VIDEO_DDRAWUSEVIDEOMEMORY:
      ddrawUseVideoMemory = !ddrawUseVideoMemory;
      regSetDwordValue("ddrawUseVideoMemory", ddrawUseVideoMemory);
      if(ddrawUseVideoMemory && ddrawEmulationOnly) {
        systemMessage(IDS_DISABLING_EMULATION_ONLY,
                      "Disabling Emulation Only setting");
        ddrawEmulationOnly = false;
        regSetDwordValue("ddrawEmulationOnly", ddrawEmulationOnly);     
      }
      systemMessage(IDS_SETTING_WILL_BE_EFFECTIVE,
                    "Setting will be effective the next time you start the emulator");
      break;      
    case ID_OPTIONS_EMULATOR_DIRECTORIES:
      showDirectories(hWindow);
      break;
    case ID_OPTIONS_EMULATOR_PAUSEWHENINACTIVE:
      pauseWhenInactive = !pauseWhenInactive;
      regSetDwordValue("pauseWhenInactive", pauseWhenInactive);
      break;
    case ID_OPTIONS_EMULATOR_DISABLESTATUSMESSAGES:
      disableStatusMessage = !disableStatusMessage;
      regSetDwordValue("disableStatus", disableStatusMessage);
      break;
    case ID_OPTIONS_EMULATOR_SYNCHRONIZE:
      synchronize = !synchronize;
      regSetDwordValue("synchronize", synchronize);
      break;
    case ID_OPTIONS_EMULATOR_SPEEDUPTOGGLE:
      speedupToggle = !speedupToggle;
      break;
    case ID_OPTIONS_EMULATOR_REMOVEINTROSGBA:
      removeIntros = !removeIntros;
      regSetDwordValue("removeIntros", removeIntros);
      break;
    case ID_OPTIONS_EMULATOR_USEBIOSFILE:
      if(biosFileName[0]) {
        useBiosFile = !useBiosFile;
        regSetDwordValue("useBios", useBiosFile);
      }
      break;
    case ID_OPTIONS_EMULATOR_SELECTBIOSFILE:
      if(emulatorSelectBiosFile()) {
        regSetStringValue("biosFile", biosFileName);
      }
      break;
    case ID_OPTIONS_EMULATOR_ASSOCIATE:
      emulatorAssociate();
      break;
    case ID_OPTIONS_EMULATOR_PNGFORMAT:
      captureFormat = 0;
      regSetDwordValue("captureFormat", 0);
      break;
    case ID_OPTIONS_EMULATOR_BMPFORMAT:
      captureFormat = 1;
      regSetDwordValue("captureFormat", 1);      
      break;
    case ID_OPTIONS_EMULATOR_SAVETYPE_AUTOMATIC:
      cpuSaveType = 0;
      regSetDwordValue("saveType", 0);
      break;
    case ID_OPTIONS_EMULATOR_SAVETYPE_EEPROM:
      cpuSaveType = 1;
      regSetDwordValue("saveType", 1);
      break;
    case ID_OPTIONS_EMULATOR_SAVETYPE_SRAM:
      cpuSaveType = 2;
      regSetDwordValue("saveType", 2);
      break;
    case ID_OPTIONS_EMULATOR_SAVETYPE_FLASH:
      cpuSaveType = 3;
      regSetDwordValue("saveType", 3);
      break;
    case ID_OPTIONS_EMULATOR_SAVETYPE_EEPROMSENSOR:
      cpuSaveType = 4;
      regSetDwordValue("saveType", 4);
      break;
    case ID_OPTIONS_SOUND_OFF:
      soundOffFlag = true;
      soundShutdown();
      regSetDwordValue("soundOff", soundOffFlag);
      break;
    case ID_OPTIONS_SOUND_MUTE:
      soundDisable(0x30f);
      regSetDwordValue("soundEnable",
                       soundGetEnable());
      break;
    case ID_OPTIONS_SOUND_ON:
      if(soundOffFlag) {
        soundOffFlag = false;
        soundInit();
      }
      soundEnable(0x30f);
      regSetDwordValue("soundEnable",
                       soundGetEnable());
      regSetDwordValue("soundOff", soundOffFlag);
      break;
    case ID_OPTIONS_SOUND_USEOLDSYNCHRONIZATION:
      useOldSync = !useOldSync;
      regSetDwordValue("useOldSync",
                       useOldSync);
      systemMessage(IDS_SETTING_WILL_BE_EFFECTIVE,
                    "Setting will be effective the next time you start the emulator");      
      break;
    case ID_OPTIONS_SOUND_ECHO:
      soundEcho = !soundEcho;
      regSetDwordValue("soundEcho", soundEcho);
      break;
    case ID_OPTIONS_SOUND_LOWPASSFILTER:
      soundLowPass = !soundLowPass;
      regSetDwordValue("soundLowPass", soundLowPass);      
      break;
    case ID_OPTIONS_SOUND_REVERSESTEREO:
      soundReverse = !soundReverse;
      regSetDwordValue("soundReverse", soundReverse);
      break;
    case ID_OPTIONS_SOUND_CHANNEL1:
    case ID_OPTIONS_SOUND_CHANNEL2:
    case ID_OPTIONS_SOUND_CHANNEL3:
    case ID_OPTIONS_SOUND_CHANNEL4:
    case ID_OPTIONS_SOUND_DIRECTSOUNDA:
    case ID_OPTIONS_SOUND_DIRECTSOUNDB:
      updateSoundChannels(wParam&0xffff);
      regSetDwordValue("soundEnable",
                       soundGetEnable());      
      break;      
    case ID_OPTIONS_SOUND_11KHZ:
      if(cartridgeType == 0)
        soundSetQuality(4);
      else
        gbSoundSetQuality(4);
      regSetDwordValue("soundQuality",
                       soundQuality);
      break;
    case ID_OPTIONS_SOUND_22KHZ:
      if(cartridgeType == 0)
        soundSetQuality(2);
      else
        gbSoundSetQuality(2);
      regSetDwordValue("soundQuality",
                       soundQuality);      
      break;
    case ID_OPTIONS_SOUND_44KHZ:
      if(cartridgeType == 0)
        soundSetQuality(1);
      else
        gbSoundSetQuality(1);
      regSetDwordValue("soundQuality",
                       soundQuality);      
      break;
    case ID_OPTIONS_SOUND_STARTRECORDING:
      fileSoundRecord();
      break;
    case ID_OPTIONS_SOUND_STOPRECORDING:
      if(soundRecorder) {
        delete soundRecorder;
        soundRecorder = NULL;
      }
      soundRecording = FALSE;
      break;
    case ID_OPTIONS_GAMEBOY_BORDER:
      gbBorderOn = !gbBorderOn;
      if(emulating && cartridgeType == 1 && gbBorderOn) {
        gbSgbRenderBorder();
      }
      updateWindowSize(videoOption);
      regSetDwordValue("borderOn", gbBorderOn);
      break;
    case ID_OPTIONS_GAMEBOY_PRINTER:
      winGbPrinterEnabled = !winGbPrinterEnabled;
      if(winGbPrinterEnabled)
        gbSerialFunction = gbPrinterSend;
      else
        gbSerialFunction = NULL;
      regSetDwordValue("gbPrinter", winGbPrinterEnabled);
      break;
    case ID_OPTIONS_GAMEBOY_AUTOMATIC:
      gbEmulatorType = 0;
      regSetDwordValue("emulatorType", gbEmulatorType);      
      break;
    case ID_OPTIONS_GAMEBOY_GBA:
      gbEmulatorType = 4;
      regSetDwordValue("emulatorType", gbEmulatorType);            
      break;
    case ID_OPTIONS_GAMEBOY_CGB:
      gbEmulatorType = 1;
      regSetDwordValue("emulatorType", gbEmulatorType);            
      break;
    case ID_OPTIONS_GAMEBOY_SGB:
      gbEmulatorType = 2;
      regSetDwordValue("emulatorType", gbEmulatorType);            
      break;
    case ID_OPTIONS_GAMEBOY_GB:
      gbEmulatorType = 3;
      regSetDwordValue("emulatorType", gbEmulatorType);            
      break;
    case ID_OPTIONS_GAMEBOY_REALCOLORS:
      gbColorOption = 0;
      regSetDwordValue("colorOption", 0);
      break;
    case ID_OPTIONS_GAMEBOY_GAMEBOYCOLORS:
      gbColorOption = 1;
      regSetDwordValue("colorOption", 1);
      break;
    case ID_OPTIONS_GAMEBOY_COLORS:
      optionsGameboyColors();
      break;
    case ID_OPTIONS_PRIORITY_HIGHEST:
      threadPriority = 0;
      regSetDwordValue("priority", 0);
      updatePriority();
      break;
    case ID_OPTIONS_PRIORITY_ABOVENORMAL:
      threadPriority = 1;
      regSetDwordValue("priority", 1);
      updatePriority();
      break;
    case ID_OPTIONS_PRIORITY_NORMAL:
      threadPriority = 2;
      regSetDwordValue("priority", 2);
      updatePriority();
      break;
    case ID_OPTIONS_PRIORITY_BELOWNORMAL:
      threadPriority = 3;
      regSetDwordValue("priority", 3);
      updatePriority();
      break;
    case ID_OPTIONS_FILTER_NORMAL:
      filterType = 0;
      regSetDwordValue("filter", filterType);
      updateFilter();
      break;
    case ID_OPTIONS_FILTER_TVMODE:
      filterType = 1;
      regSetDwordValue("filter", filterType);
      updateFilter();
      break;
    case ID_OPTIONS_FILTER_2XSAI:
      filterType = 2;
      regSetDwordValue("filter", filterType);
      updateFilter();
      break;
    case ID_OPTIONS_FILTER_SUPER2XSAI:
      filterType = 3;
      regSetDwordValue("filter", filterType);
      updateFilter();
      break;
    case ID_OPTIONS_FILTER_SUPEREAGLE:
      filterType = 4;
      regSetDwordValue("filter", filterType);
      updateFilter();
      break;
    case ID_OPTIONS_FILTER16BIT_PIXELATEEXPERIMENTAL:
      filterType = 5;
      regSetDwordValue("filter", filterType);
      updateFilter();
      break;
    case ID_OPTIONS_FILTER16BIT_MOTIONBLUREXPERIMENTAL:
      filterType = 6;
      regSetDwordValue("filter", filterType);
      updateFilter();
      break;
    case ID_OPTIONS_FILTER16BIT_ADVANCEMAMESCALE2X:
      filterType = 7;
      regSetDwordValue("filter", filterType);
      updateFilter();
      break;
    case ID_OPTIONS_FILTER16BIT_SIMPLE2X:
      filterType = 8;
      regSetDwordValue("filter", filterType);
      updateFilter();
      break;
    case ID_OPTIONS_FILTER_DISABLEMMX:
      disableMMX = !disableMMX;
      if(!disableMMX)
        cpu_mmx = detectMMX();
      else
        cpu_mmx = 0;
      regSetDwordValue("disableMMX", disableMMX);
      break;
    case ID_OPTIONS_JOYPAD:
      configurePad();
      break;
    case ID_OPTIONS_JOYPAD_MOTIONCONFIGURE:
      motionConfigurePad();
      break;
    case ID_OPTIONS_JOYPAD_AUTOFIRE_A:
      if(autoFire & 1) {
        autoFire &= ~1;
        systemScreenMessage((char *)winResLoadString(IDS_AUTOFIRE_A_DISABLED));
      } else {
        autoFire |= 1;
        systemScreenMessage((char *)winResLoadString(IDS_AUTOFIRE_A));
      }
      break;
    case ID_OPTIONS_JOYPAD_AUTOFIRE_B:
      if(autoFire & 2) {
        autoFire &= ~2;
        systemScreenMessage((char *)winResLoadString(IDS_AUTOFIRE_B_DISABLED));
      } else {
        autoFire |= 2;
        systemScreenMessage((char *)winResLoadString(IDS_AUTOFIRE_B));
      }
      break;
    case ID_OPTIONS_JOYPAD_AUTOFIRE_L:
      if(autoFire & 512) {
        autoFire &= ~512;
        systemScreenMessage((char *)winResLoadString(IDS_AUTOFIRE_L_DISABLED));
      } else {
        autoFire |= 512;
        systemScreenMessage((char *)winResLoadString(IDS_AUTOFIRE_L));
      }
      break;
    case ID_OPTIONS_JOYPAD_AUTOFIRE_R:
      if(autoFire & 256) {
        autoFire &= ~256;
        systemScreenMessage((char *)winResLoadString(IDS_AUTOFIRE_R_DISABLED));
      } else {
        autoFire |= 256;
        systemScreenMessage((char *)winResLoadString(IDS_AUTOFIRE_R));
      }
      break;
    case ID_OPTIONS_LANGUAGE_SYSTEM:
      winSetLanguageOption(0, false);
      break;
    case ID_OPTIONS_LANGUAGE_ENGLISH:
      winSetLanguageOption(1, false);
      break;
    case ID_OPTIONS_LANGUAGE_OTHER:
      winSetLanguageOption(2, false);
      break;
    case ID_CHEATS_SEARCHFORCHEATS:
      if(cartridgeType == 0)
        winCheatsDialog();
      else
        winGbCheatsDialog();
      break;
    case ID_CHEATS_CHEATLIST:
      if(cartridgeType == 0)
        winCheatsListDialog();
      else
        winGbCheatsListDialog();
      break;
    case ID_DEBUG_NEXTFRAME:
      if(paused)
        paused=FALSE;
      winPauseNextFrame = TRUE;
      break;
    case ID_CHEATS_AUTOMATICSAVELOADCHEATS:
      autoSaveLoadCheatList = !autoSaveLoadCheatList;
      regSetDwordValue("autoSaveCheatList", autoSaveLoadCheatList);
      break;
    case ID_CHEATS_LOADCHEATLIST:
      winLoadCheatList();
      break;
    case ID_CHEATS_SAVECHEATLIST:
      winSaveCheatList();
      break;
    case ID_TOOLS_DISASSEMBLE:
      toolsDisassemble();
      break;
    case ID_TOOLS_LOGGING:
      toolsLogging();
      break;
    case ID_TOOLS_MAPVIEW:
      toolsMapView();
      break;
    case ID_TOOLS_MEMORYVIEWER:
      toolsMemoryViewer();
      break;
    case ID_TOOLS_PALETTEVIEW:
      toolsPaletteView();
      break;
    case ID_TOOLS_OAMVIEWER:
      toolsOamViewer();
      break;
    case ID_TOOLS_TILEVIEWER:
      toolsTileViewer();
      break;
    case ID_TOOLS_CUSTOMIZE:
      toolsCustomize();
      break;
    case ID_TOOLS_DEBUG_GDB:
      toolsDebugGDB();
      break;
    case ID_TOOLS_DEBUG_LOADANDWAIT:
      toolsDebugGDBLoad();
      break;
    case ID_TOOLS_DEBUG_DISCONNECT:
      remoteCleanUp();
      debugger = false;
      break;
    case ID_TOOLS_DEBUG_BREAK:
      if(armState) {
        armNextPC -= 4;
        reg[15].I -= 4;
      } else {
        armNextPC -= 2;
        reg[15].I -= 2;
      }
      debugger = true;
      break;
    case ID_HELP_ABOUT:
      helpAbout();
      break;
    }
    return 0L;
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

HRESULT WINAPI checkModesAvailable(LPDDSURFACEDESC2 surf, LPVOID lpContext)
{
  if(surf->dwWidth == 320 &&
     surf->dwHeight == 240 &&
     surf->ddpfPixelFormat.dwRGBBitCount == 16) {
    mode320Available = TRUE;
  }
  if(surf->dwWidth == 640 &&
     surf->dwHeight == 480 &&
     surf->ddpfPixelFormat.dwRGBBitCount == 16) {
    mode640Available = TRUE;
  }
  if(surf->dwWidth == 800 &&
     surf->dwHeight == 600 &&
     surf->ddpfPixelFormat.dwRGBBitCount == 16) {
    mode800Available = TRUE;
  }
  return DDENUMRET_OK;
}

BOOL initDirectDraw()
{
  sizeX = 240;
  sizeY = 160;

  switch(videoOption) {
  case VIDEO_1X:
    surfaceSizeX = sizeX;
    surfaceSizeY = sizeY;
    break;
  case VIDEO_2X:
    surfaceSizeX = sizeX * 2;
    surfaceSizeY = sizeY * 2;
    break;
  case VIDEO_3X:
    surfaceSizeX = sizeX * 3;
    surfaceSizeY = sizeY * 3;
    break;
  case VIDEO_4X:
    surfaceSizeX = sizeX * 4;
    surfaceSizeY = sizeY * 4;
    break;
  case VIDEO_320x240:
    surfaceSizeX = sizeX;
    surfaceSizeY = sizeY;
    if(fullScreenStretch) {
      surfaceSizeX = 320;
      surfaceSizeY = 240;
    }
    break;
  case VIDEO_640x480:
    surfaceSizeX = sizeX * 2;
    surfaceSizeY = sizeY * 2;
    if(fullScreenStretch) {
      surfaceSizeX = 640;
      surfaceSizeY = 480;
    }    
    break;
  case VIDEO_800x600:
    surfaceSizeX = sizeX * 3;
    surfaceSizeY = sizeY * 3;
    if(fullScreenStretch) {
      surfaceSizeX = 800;
      surfaceSizeY = 600;
    }    
    break;
  }
  
  rect.left = 0;
  rect.top = 0;
  rect.right = sizeX;
  rect.bottom = sizeY;

  dest.left = 0;
  dest.top = 0;
  dest.right = surfaceSizeX;
  dest.bottom = surfaceSizeY;

  DWORD style = WS_POPUP | WS_VISIBLE;
  DWORD styleEx = 0;
  
  if(videoOption <= VIDEO_4X)
    style |= WS_OVERLAPPEDWINDOW;
  else
    styleEx = WS_EX_TOPMOST;

  if(videoOption <= VIDEO_4X)
    AdjustWindowRectEx(&dest, style, TRUE, styleEx);
  else
    AdjustWindowRectEx(&dest, style, FALSE, styleEx);    

  int winSizeX = dest.right-dest.left;
  int winSizeY = dest.bottom-dest.top;

  int x = 0;
  int y = 0;

  if(videoOption <= VIDEO_4X) {
    x = windowPositionX;
    y = windowPositionY;
  }
  
  // Create a window
  hWindow = CreateWindowEx(styleEx,
                           "GBA",
                           "VisualBoyAdvance",
                           style,
                           x,
                           y,
                           winSizeX,
                           winSizeY,
                           NULL,
                           NULL,
                           hInstance,
                           NULL);
  
  if (!hWindow) {
    log("Error creating Window %08x\n", GetLastError());
    //    errorMessage(myLoadString(IDS_ERROR_DISP_FAILED));
    return FALSE;
  }
  
  ShowWindow(hWindow, nCmdShow);
  UpdateWindow(hWindow);
  SetFocus(hWindow);

  updateMenuBar();
  
  adjustDestRect();
  
  GUID *guid = NULL;
  if(ddrawEmulationOnly)
    guid = (GUID *)DDCREATE_EMULATIONONLY;

  ddrawDLL = LoadLibrary("DDRAW.DLL");
  HRESULT (WINAPI *DDrawCreateEx)(GUID *,LPVOID *,REFIID,IUnknown *);  
  if(ddrawDLL != NULL) {    
    DDrawCreateEx = (HRESULT (WINAPI *)(GUID *,LPVOID *,REFIID,IUnknown *))
      GetProcAddress(ddrawDLL, "DirectDrawCreateEx");

    if(DDrawCreateEx == NULL) {
      directXMessage("DirectDrawCreateEx");
      return FALSE;
    }
  } else {
    directXMessage("DDRAW.DLL");
    return FALSE;
  }
  
  HRESULT hret = DDrawCreateEx(guid,
                               (void **)&pDirectDraw,
                               IID_IDirectDraw7,
                               NULL);
    
  if(hret != DD_OK) {
    log("Error creating DirectDraw object %08x\n", hret);
    if(ddrawEmulationOnly) {
      // disable emulation only setting in case of failure
      regSetDwordValue("ddrawEmulationOnly", 0);
    }
    //    errorMessage(myLoadString(IDS_ERROR_DISP_DRAWCREATE), hret);
    return FALSE;
  }

  mode320Available = FALSE;
  mode640Available = FALSE;
  // check for available fullscreen modes
  pDirectDraw->EnumDisplayModes(DDEDM_STANDARDVGAMODES, NULL, NULL,
                                checkModesAvailable);
  
  DWORD flags = DDSCL_NORMAL;

  if(videoOption >= VIDEO_320x240)
    flags = DDSCL_ALLOWMODEX |
      DDSCL_ALLOWREBOOT |
      DDSCL_EXCLUSIVE |
      DDSCL_FULLSCREEN;
  
  hret = pDirectDraw->SetCooperativeLevel(hWindow,  
                                          flags);

  if(hret != DD_OK) {
    log("Error SetCooperativeLevel %08x\n", hret);    
    //    errorMessage(myLoadString(IDS_ERROR_DISP_DRAWLEVEL), hret);
    return FALSE;
  }

  if(videoOption == VIDEO_640x480) {
    hret = pDirectDraw->SetDisplayMode(640,
                                       480,
                                       16,
                                       0,
                                       0);
    
    if(hret != DD_OK) {
      log("Error SetDisplayMode %08x\n", hret);      
      videoOption = VIDEO_1X;
      regSetDwordValue("video", videoOption);
      //      errorMessage(myLoadString(IDS_ERROR_DISP_DRAWSET), hret);
      return FALSE;
    }
  }
  if(videoOption == VIDEO_320x240) {
    hret = pDirectDraw->SetDisplayMode(320,
                                       240,
                                       16,
                                       0,
                                       0);
    if(hret != DD_OK) {
      log("Error SetDisplayMode %08x\n", hret);      
      videoOption = VIDEO_1X;
      regSetDwordValue("video", videoOption);
      //      errorMessage(myLoadString(IDS_ERROR_DISP_DRAWSET), hret);
      return FALSE;
    }
  }

  if(videoOption == VIDEO_800x600) {
    hret = pDirectDraw->SetDisplayMode(800,
                                       600,
                                       16,
                                       0,
                                       0);
    if(hret != DD_OK) {
      log("Error SetDisplayMode %08x\n", hret);      
      videoOption = VIDEO_1X;
      regSetDwordValue("video", videoOption);
      //      errorMessage(myLoadString(IDS_ERROR_DISP_DRAWSET), hret);
      return FALSE;
    }
  }
  
  DDSURFACEDESC2 ddsd;
  ZeroMemory(&ddsd,sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags = DDSD_CAPS;
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
  
  hret = pDirectDraw->CreateSurface(&ddsd, &ddsPrimary, NULL);
  if(hret != DD_OK) {
    log("Error primary CreateSurface %08x\n", hret);    
    //    errorMessage(myLoadString(IDS_ERROR_DISP_DRAWSURFACE), hret);
    return FALSE;
  }

  // create clipper in all modes to avoid paint problems
  //  if(videoOption <= VIDEO_4X) {
    hret = pDirectDraw->CreateClipper(0, &ddsClipper, NULL);
    if(hret == DD_OK) {
      ddsClipper->SetHWnd(0, hWindow);
      ddsPrimary->SetClipper(ddsClipper);
    }
    //  }

  ZeroMemory(&ddsd, sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
  ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
  if(ddrawUseVideoMemory)
    ddsd.ddsCaps.dwCaps |= (DDSCAPS_LOCALVIDMEM|DDSCAPS_VIDEOMEMORY);
  if(videoOption == VIDEO_320x240) {
    ddsd.dwWidth = 320;
    ddsd.dwHeight = 240;
  } else {
    ddsd.dwWidth = 256*2; //240;  
    ddsd.dwHeight = 240*2; //160;
  }

  hret = pDirectDraw->CreateSurface(&ddsd, &ddsOffscreen, NULL);

  if(hret != DD_OK) {
    log("Error offscreen CreateSurface %08x\n", hret);    
    if(ddrawUseVideoMemory) {
      regSetDwordValue("ddrawUseVideoMemory", 0);
    }    
    //    errorMessage(myLoadString(IDS_ERROR_DISP_DRAWSURFACE2), hret);
    return FALSE;
  }

  
  ZeroMemory(&ddsd, sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  
  hret = ddsOffscreen->GetSurfaceDesc(&ddsd);
  
  DDPIXELFORMAT px;

  px.dwSize = sizeof(px);

  hret = ddsOffscreen->GetPixelFormat(&px);

  switch(px.dwRGBBitCount) {
  case 15:
  case 16:
    systemColorDepth = 16;
    break;
  case 24:
    systemColorDepth = 24;
    filterFunction = NULL;
    break;
  case 32:
    systemColorDepth = 32;
    filterFunction = NULL;
    break;
  default:
    systemMessage(IDS_ERROR_DISP_COLOR, "Unsupported display setting for color depth: %d bits. \nWindows desktop must be in either 16-bit, 24-bit or 32-bit mode for this program to work in window mode.",px.dwRGBBitCount);
    return FALSE;
  }

  systemRedShift = ffs(px.dwRBitMask);
  systemGreenShift = ffs(px.dwGBitMask);
  systemBlueShift = ffs(px.dwBBitMask);

#ifdef MMX
  if(!disableMMX)
    cpu_mmx = detectMMX();
  else
    cpu_mmx = 0;
#endif
  
  if((px.dwFlags&DDPF_RGB) != 0 &&
     px.dwRBitMask == 0xF800 &&
     px.dwGBitMask == 0x07E0 &&
     px.dwBBitMask == 0x001F) {
    systemGreenShift++;
    Init_2xSaI(565);
    RGB_LOW_BITS_MASK=0x821;
  } else if((px.dwFlags&DDPF_RGB) != 0 &&
            px.dwRBitMask == 0x7C00 &&
            px.dwGBitMask == 0x03E0 &&
            px.dwBBitMask == 0x001F) {
    Init_2xSaI(555);
    RGB_LOW_BITS_MASK=0x421;
  } else if((px.dwFlags&DDPF_RGB) != 0 &&
            px.dwRBitMask == 0x001F &&
            px.dwGBitMask == 0x07E0 &&
            px.dwBBitMask == 0xF800) {
    systemGreenShift++;
    Init_2xSaI(565);
    RGB_LOW_BITS_MASK=0x821;
  } else if((px.dwFlags&DDPF_RGB) != 0 &&
            px.dwRBitMask == 0x001F &&
            px.dwGBitMask == 0x03E0 &&
            px.dwBBitMask == 0x7C00) {
    Init_2xSaI(555);
    RGB_LOW_BITS_MASK=0x421;
  } else {
    // 32-bit or 24-bit
    if(systemColorDepth == 32 || systemColorDepth == 24) {
      systemRedShift += 3;
      systemGreenShift += 3;
      systemBlueShift += 3;
    }
  }

  switch(systemColorDepth) {
  case 16:
    {
      for(int i = 0; i < 0x10000; i++) {
        systemColorMap16[i] = ((i & 0x1f) << systemRedShift) |
          (((i & 0x3e0) >> 5) << systemGreenShift) |
          (((i & 0x7c00) >> 10) << systemBlueShift);
      }
    }
    break;
  case 24:
  case 32:
    {
      for(int i = 0; i < 0x10000; i++) {
        systemColorMap32[i] = ((i & 0x1f) << systemRedShift) |
          (((i & 0x3e0) >> 5) << systemGreenShift) |
          (((i & 0x7c00) >> 10) << systemBlueShift);
      }      
    }
    break;
  }
  for(int i = 0; i < 24;) {
    systemGbPalette[i++] = (0x1f) | (0x1f << 5) | (0x1f << 10);
    systemGbPalette[i++] = (0x15) | (0x15 << 5) | (0x15 << 10);
    systemGbPalette[i++] = (0x0c) | (0x0c << 5) | (0x0c << 10);
    systemGbPalette[i++] = 0;
  }
  updateFilter();
  
  DragAcceptFiles(hWindow, TRUE);
  
  return TRUE;
}

#define PACKVERSION(major,minor) MAKELONG(minor,major)

DWORD GetDllVersion(LPCTSTR lpszDllName)
{

    HINSTANCE hinstDll;
    DWORD dwVersion = 0;

    hinstDll = LoadLibrary(lpszDllName);
        
    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;

        pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");

/*Because some DLLs may not implement this function, you
 *must test for it explicitly. Depending on the particular 
 *DLL, the lack of a DllGetVersion function may
 *be a useful indicator of the version.
*/
        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
                dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }
        
        FreeLibrary(hinstDll);
    }
    return dwVersion;
}

BOOL initApp(HINSTANCE hInstance, int nCmdShow)
{
  remoteSetProtocol(0);
  
  if(GetDllVersion("comctl32.dll") < PACKVERSION(4,71)) {
    systemMessage(0, "Error: COMCTL32.DLL needs to be at least version 4.71");
    return FALSE;
  }
  memset(delta,255, 257*244*2);
  systemVerbose = GetPrivateProfileInt("config",
                                       "verbose",
                                       0,
                                       "VBA.ini");
  
  systemDebug = GetPrivateProfileInt("config",
                                     "debug",
                                     0,
                                     "VBA.ini");
  
  WNDCLASS                    wc;
 
  // Set up and register window class
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON));
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH )GetStockObject(BLACK_BRUSH);
  wc.lpszMenuName = "GBA";
  wc.lpszClassName = "GBA";
  RegisterClass(&wc);

  ::hInstance = hInstance;
  ::nCmdShow = nCmdShow;

  arrow = LoadCursor(NULL, IDC_ARROW);
  
  regInit();

  languageOption = regQueryDwordValue("language", 1);
  if(languageOption < 0 || languageOption > 2)
    languageOption = 1;

  char *lang = regQueryStringValue("languageName", NULL);
  if(lang) {
    strncpy(languageName, lang, 4);
    languageName[3] = 0;
  } else
    languageName[0] = 0;
  
  winSetLanguageOption(languageOption, true);
  
  frameSkip = regQueryDwordValue("frameSkip", 2);
  if(frameSkip < 0 || frameSkip > 9)
    frameSkip = 2;

  gbFrameSkip = regQueryDwordValue("gbFrameSkip", 0);
  if(gbFrameSkip < 0 || gbFrameSkip > 9)
    gbFrameSkip = 0;
  
  vsync = regQueryDwordValue("vsync", 0);
  synchronize = regQueryDwordValue("synchronize", 1) ? true : false;
  fullScreenStretch = regQueryDwordValue("stretch", 0) ? true : false;

  videoOption = regQueryDwordValue("video", 0);

  if(videoOption < 0 || videoOption > VIDEO_800x600)
    videoOption = 0;

  windowPositionX = regQueryDwordValue("windowX", 0);
  if(windowPositionX < 0)
    windowPositionX = 0;
  windowPositionY = regQueryDwordValue("windowY", 0);
  if(windowPositionY < 0)
    windowPositionY = 0;

  useBiosFile = regQueryDwordValue("useBios", 0) ? true: false;

  char * c = regQueryStringValue("biosFile", NULL);

  if(c) {
    strcpy(biosFileName, c);
  }
  
  int res = regQueryDwordValue("soundEnable", 0x30f);
  
  soundEnable(res);
  soundDisable(~res);

  soundOffFlag = (regQueryDwordValue("soundOff", 0)) ? true : false;

  soundQuality = regQueryDwordValue("soundQuality", 2);

  soundEcho = regQueryDwordValue("soundEcho", 0) ? true : false;

  soundLowPass = regQueryDwordValue("soundLowPass", 0) ? true : false;

  soundReverse = regQueryDwordValue("soundReverse", 0) ? true : false;

  ddrawEmulationOnly = regQueryDwordValue("ddrawEmulationOnly", 0);
  ddrawUseVideoMemory = regQueryDwordValue("ddrawUseVideoMemory", 0);

  filterType = regQueryDwordValue("filter", 0);
  if(filterType < 0 || filterType > 8)
    filterType = 0;

  disableMMX = regQueryDwordValue("disableMMX", 0) ? true: false;

  disableStatusMessage = regQueryDwordValue("disableStatus", 0) ? true : false;

  winGbPrinterEnabled = regQueryDwordValue("gbPrinter", 0);

  pauseWhenInactive = regQueryDwordValue("pauseWhenInactive", 1) ?
    true : false;

  useOldSync = regQueryDwordValue("useOldSync", 0) ?
    TRUE : FALSE;

  captureFormat = regQueryDwordValue("captureFormat", 0);

  removeIntros = regQueryDwordValue("removeIntros", 0);

  recentFreeze = regQueryDwordValue("recentFreeze", 0);

  cpuSaveType = regQueryDwordValue("saveType", 0);
  if(cpuSaveType < 0 || cpuSaveType > 4)
    cpuSaveType = 0;

  if(!initDirectDraw()) {
    if(videoOption >= VIDEO_320x240)
      regSetDwordValue("video", VIDEO_1X);    
    return FALSE;
  }
  if(!initDirectInput())
    return FALSE;

  readKeys();

  if(regQueryDwordValue("joyVersion", 0) == 0) {
    convertKeys();
  }
  
  checkKeys();

  hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));

  winAccelMgr.Connect(hWindow);

  winAccelMgr.SetRegKey(HKEY_CURRENT_USER, "Software\\Emulators\\VisualBoyAdvance");

  extern void winAccelAddCommands(CAcceleratorManager&);

  winAccelAddCommands(winAccelMgr);

  winAccelMgr.CreateDefaultTable();

  winAccelMgr.Load();

  winAccelMgr.UpdateWndTable();

  char buffer[50];
  
  for(int i = 0; i < 10; i++) {
    sprintf(buffer, "recent%d", i);
    char *s = regQueryStringValue(buffer, NULL);
    if(s == NULL)
      break;
    recentFiles[i] = strdup(s);
  }
  
  gbBorderOn = regQueryDwordValue("borderOn", 0);
  gbEmulatorType = regQueryDwordValue("emulatorType", 1);
  if(gbEmulatorType < 0 || gbEmulatorType > 4)
    gbEmulatorType = 1;
  gbColorOption = regQueryDwordValue("colorOption", 0);

  threadPriority = regQueryDwordValue("priority", 2);

  if(threadPriority < 0 || threadPriority >3)
    threadPriority = 2;
  updatePriority();

  autoSaveLoadCheatList = regQueryDwordValue("autoSaveCheatList", 0) ?
    true : false;

  gbPaletteOption = regQueryDwordValue("gbPaletteOption", 0);
  if(gbPaletteOption < 0)
    gbPaletteOption = 0;
  if(gbPaletteOption > 2)
    gbPaletteOption = 2;

  regQueryBinaryValue("gbPalette", (char *)systemGbPalette,
                      24*sizeof(u16));
  return TRUE;
}

void fileToggleMenu()
{
  if(videoOption <= VIDEO_4X)
    return;
  
  menuToggle = !menuToggle;

  BOOL res = FALSE;
  
  if(menuToggle) {
    updateMenuBar();
  } else {
    res = SetMenu(hWindow,NULL);
    DestroyMenu(menu);
  }

  if(res != TRUE) {
    DWORD error = GetLastError();
  }
  
  adjustDestRect();
}

void fileExit()
{
  //  saveVideoOptions();    
  //  shutdownEmulation();
  //  releaseAllObjects();
  PostQuitMessage(0);
  PostMessage(hWindow, WM_CLOSE, 0, 0);
  
  //  if(out)
  //    fclose(out);
}

char *getDirFromFile(char *file)
{
  static char buffer[2048];

  strcpy(buffer, file);
  char *p = strrchr(buffer, '\\');
  if(p) {
    *p = 0;
    if(strlen(buffer) == 2 && buffer[1] == ':')
      strcat(buffer, "\\");
  } else {
    buffer[0] = 0;
  }
  return buffer;
}

BOOL fileOpenSelect()
{
  OPENFILENAME ofn;

  paused = FALSE;
  
  dir[0] = 0;

  char * initDir = regQueryStringValue("romdir", NULL);

  if(initDir != NULL)
    strcpy((char *)dir,initDir);

  int selectedFile = regQueryDwordValue("selectedFilter", 0);
  if(selectedFile < 0 || selectedFile > 2)
    selectedFile = 0;
  
  szFile[0] = 0;
  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWindow;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter =  winLoadFilter(IDS_FILTER_ROM);
  ofn.nFilterIndex = selectedFile;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = (const char *)dir;
  ofn.lpstrTitle = winResLoadString(IDS_SELECT_ROM); //szSelectRom;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if(videoOption == VIDEO_320x240) {
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG);
    ofn.Flags |= OFN_ENABLETEMPLATE;
  }
  
  if(GetOpenFileName(&ofn) == FALSE) {
    DWORD res = CommDlgExtendedError();
    return FALSE;
  }

  regSetDwordValue("selectedFilter", ofn.nFilterIndex);
    
  if(ofn.nFileOffset > 0) {
    CopyMemory(dir,szFile,ofn.nFileOffset);
    dir[ofn.nFileOffset] = 0;
  }

  int len = strlen((const char *)dir);
  if(len > 3 && dir[len-1] == '\\')
    dir[len-1] = 0;
  
  regSetStringValue("romdir",
                    (char *)dir);

  return TRUE;
}

BOOL fileOpenSelectGB()
{
  OPENFILENAME ofn;

  paused = FALSE;
  
  dir[0] = 0;

  char * initDir = regQueryStringValue("gbromdir", NULL);

  if(initDir != NULL)
    strcpy((char *)dir,initDir);

  int selectedFile = 0;
  
  szFile[0] = 0;
  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWindow;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter =  winLoadFilter(IDS_FILTER_GBROM);
  ofn.nFilterIndex = selectedFile;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = (const char *)dir;
  ofn.lpstrTitle = winResLoadString(IDS_SELECT_ROM); //szSelectRom;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if(videoOption == VIDEO_320x240) {
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG);
    ofn.Flags |= OFN_ENABLETEMPLATE;
  }
  
  if(GetOpenFileName(&ofn) == FALSE) {
    DWORD res = CommDlgExtendedError();
    return FALSE;
  }

  if(ofn.nFileOffset > 0) {
    CopyMemory(dir,szFile,ofn.nFileOffset);
    dir[ofn.nFileOffset] = 0;
  }

  int len = strlen((const char *)dir);
  if(len > 3 && dir[len-1] == '\\')
    dir[len-1] = 0;
  
  regSetStringValue("gbromdir",
                    (char *)dir);

  return TRUE;
}

BOOL fileOpen()
{
  char buffer[2048];

  // save battery file before we change the filename...
  if(rom != NULL || gbRom != NULL) {
    if(autoSaveLoadCheatList)
      winSaveCheatListDefault();
    writeBatteryFile();
    emuCleanUp();
    remoteCleanUp();    
  }

  _fullpath(filename, szFile, 1024);

  char *p = strrchr(filename, '.');

  if(p != NULL)
    *p = 0;  

  if(!dir[0]) {
    p = strrchr(filename, '\\');
    if(p) {
      strncpy(dir, filename, p-filename);
      dir[p-filename] = 0;
    }
  }
  
  if(CPUIsZipFile(szFile)) {
    unzFile unz = unzOpen(szFile);

    if(unz == NULL) {
      systemMessage(IDS_FAILED_TO_OPEN_FILE, "Failed to open file %s", szFile);
      return FALSE;
    }

    int r = unzGoToFirstFile(unz);

    if(r != UNZ_OK) {
      unzClose(unz);
      systemMessage(IDS_FAILED_TO_READ_ZIP_DIR,
                    "Failed to read zip directory for file %s", szFile);
      return FALSE;
    }

    bool found = false;

    unz_file_info info;
    
    while(1) {
      r = unzGetCurrentFileInfo(unz,
                                &info,
                                buffer,
                                sizeof(buffer),
                                NULL,
                                0,
                                NULL,
                                0);
      if(r != UNZ_OK) {
        unzClose(unz);
        systemMessage(IDS_FAILED_TO_READ_ZIP_DIR,
                      "Failed to read zip directory for file %s", szFile);
        return false;
      }

      if(gbIsGameboyRom(buffer)) {
        cartridgeType = 1;
        found = true;
        break;
      }
      if(CPUIsGBAImage(buffer)) {
        cartridgeType = 0;
        found = true;
        break;
      }

      r = unzGoToNextFile(unz);

      if(r != UNZ_OK)
        break;
    }

    unzClose(unz);
    
    if(!found) {
      systemMessage(MSG_NO_IMAGE_ON_ZIP,
                    "No image to found on ZIP file %s", szFile);
      return FALSE;
    }
  } else if(gbIsGameboyRom(szFile)) {
    cartridgeType = 1;
  } else if(CPUIsGBAImage(szFile)) {
    cartridgeType = 0;
  } else {
    systemMessage(IDS_UNSUPPORTED_FILE_TYPE,
                  "Unsupported file type: %s", szFile);
    return FALSE;
  }

  if(cartridgeType == 1) {
    if(!gbLoadRom(szFile))
      return FALSE;
    emuWriteState = gbWriteSaveState;
    emuReadState = gbReadSaveState;
    emuWriteBattery = gbWriteBatteryFile;
    emuReadBattery = gbReadBatteryFile;
    emuReset = gbReset;
    emuCleanUp = gbCleanUp;
    emuWritePNG = gbWritePNGFile;
    emuWriteBMP = gbWriteBMPFile;
    emuMain = gbEmulate;
#ifdef FINAL_VERSION
    emuCount = 70000/4;
#else
    emuCount = 1000;
#endif
  } else {
    if(!CPULoadRom(szFile))
      return FALSE;
    
    emuWriteState = CPUWriteState;
    emuReadState = CPUReadState;
    emuWriteBattery = CPUWriteBatteryFile;
    emuReadBattery = CPUReadBatteryFile;
    emuReset = CPUReset;
    emuCleanUp = CPUCleanUp;
    emuWritePNG = CPUWritePNGFile;
    emuWriteBMP = CPUWriteBMPFile;
    emuMain = CPULoop;
#ifdef FINAL_VERSION
    emuCount = 250000;
#else
    emuCount = 5000;
#endif
    if(removeIntros && rom != NULL) {
      *((u32 *)rom)= 0xea00002e;
    }
  }
    
  if(soundInitialized) {
    if(cartridgeType == 1)
      gbSoundReset();
    else
      soundReset();
  } else {
    if(!soundOffFlag)
      soundInit();
    soundInitialized = TRUE;
  }

  if(cartridgeType == 0) {
    CPUInit(biosFileName, useBiosFile ? true : false);
    CPUReset();
  }
  
  readBatteryFile();

  if(autoSaveLoadCheatList)
    winLoadCheatListDefault();
  
  addRecentFile(szFile);
  
  updateWindowSize(videoOption);
  
  BOOL flag = TRUE;
  emulating = TRUE;

  if(screenSaverState)
    setScreenSaverEnable(FALSE);
  
  return TRUE;
}

void loadSaveGame()
{
  OPENFILENAME ofn;

  char * p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  char * saveDir = regQueryStringValue("saveDir", NULL);

  if(!saveDir)
    saveDir = getDirFromFile(filename);
  
  if(isDriveRoot(saveDir))
    sprintf(szFile, "%s%s.sgm", saveDir, p);
  else
    sprintf(szFile, "%s\\%s.sgm", saveDir, p);
  
  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWindow;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = winLoadFilter(IDS_FILTER_SGM);
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = (const char *)saveDir;
  ofn.lpstrTitle = winResLoadString(IDS_SELECT_SAVE_GAME_NAME);
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if(videoOption == VIDEO_320x240) {
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG);
    ofn.Flags |= OFN_ENABLETEMPLATE;
  }
  
  if(GetOpenFileName(&ofn) == FALSE) {
    DWORD res = CommDlgExtendedError();
    return;
  }

  emuReadState(ofn.lpstrFile);
  
  systemScreenMessage((char *)winResLoadString(IDS_LOADED_STATE));  
}

void loadSaveGame(int num)
{
  char * p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  char * saveDir = regQueryStringValue("saveDir", NULL);

  if(!saveDir)
    saveDir = getDirFromFile(filename);

  if(isDriveRoot(saveDir))
    sprintf(szFile, "%s%s%d.sgm", saveDir, p, num);
  else
    sprintf(szFile, "%s\\%s%d.sgm", saveDir, p, num);

  emuReadState(szFile);

  sprintf(winBuffer, winResLoadString(IDS_LOADED_STATE_N), num);
  
  systemScreenMessage(winBuffer);
}

void writeSaveGame()
{
  OPENFILENAME ofn;

  char *p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  char *saveDir = regQueryStringValue("saveDir", NULL);
  if(!saveDir)
    saveDir = getDirFromFile(filename);

  if(isDriveRoot(saveDir))
    sprintf(szFile, "%s%s.sgm", saveDir, p);
  else
    sprintf(szFile, "%s\\%s.sgm", saveDir, p);

  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWindow;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter =  winLoadFilter(IDS_FILTER_SGM);
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = NULL;
  ofn.lpstrDefExt = "SGM";
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = (const char *)saveDir;
  ofn.lpstrTitle = winResLoadString(IDS_SELECT_SAVE_GAME_NAME);
  ofn.Flags = OFN_PATHMUSTEXIST;

  if(videoOption == VIDEO_320x240) {
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG);
    ofn.Flags |= OFN_ENABLETEMPLATE;
  }
  
  if(GetSaveFileName(&ofn) == FALSE) {
    DWORD res = CommDlgExtendedError();
    return;
  }

  emuWriteState(ofn.lpstrFile);

  systemScreenMessage((char *)winResLoadString(IDS_WROTE_STATE));
}

void writeSaveGame(int num)
{
  char * p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  char * saveDir = regQueryStringValue("saveDir", NULL);

  if(!saveDir)
    saveDir = getDirFromFile(filename);

  if(isDriveRoot(saveDir))
    sprintf(szFile, "%s%s%d.sgm", saveDir, p, num);
  else
    sprintf(szFile, "%s\\%s%d.sgm", saveDir, p, num);

  emuWriteState(szFile);

  sprintf(winBuffer, winResLoadString(IDS_WROTE_STATE_N), num);
  systemScreenMessage(winBuffer);
}

void writeBatteryFile()
{
  char buffer[1048];
  
  char *p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  char *saveDir = regQueryStringValue("batteryDir", NULL);
  if(!saveDir)
    saveDir = getDirFromFile(filename);

  if(isDriveRoot(saveDir))
    sprintf(buffer, "%s%s.sav", saveDir, p);
  else
    sprintf(buffer, "%s\\%s.sav", saveDir, p);

  emuWriteBattery(buffer);
}

void readBatteryFile()
{
  char buffer[1048];
  char *p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  char *saveDir = regQueryStringValue("batteryDir", NULL);
  if(!saveDir)
    saveDir = getDirFromFile(filename);

  if(isDriveRoot(saveDir))
    sprintf(buffer, "%s%s.sav", saveDir, p);
  else
    sprintf(buffer, "%s\\%s.sav", saveDir, p);

  bool res;

  res = emuReadBattery(buffer);

  if(res)
    systemScreenMessage((char *)winResLoadString(IDS_LOADED_BATTERY));
}

BOOL IsIdleMessage(MSG* pMsg)
{
  // Return FALSE if the message just dispatched should _not_
  // cause OnIdle to be run.  Messages which do not usually
  // affect the state of the user interface and happen very
  // often are checked for.
  
  // redundant WM_MOUSEMOVE and WM_NCMOUSEMOVE
  if (pMsg->message == WM_MOUSEMOVE || pMsg->message == WM_NCMOUSEMOVE)
    return FALSE;
  
  // WM_PAINT and WM_SYSTIMER (caret blink)
  return pMsg->message != WM_PAINT && pMsg->message != 0x0118;
}

BOOL OnIdle(LONG count)
{
  if(emulating && debugger) {
    MSG msg;    
    remoteStubMain();
    if(debugger)
      return TRUE; // continue loop
    return !::PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE);
  } else if(emulating && active && !paused) {
    emuMain(emuCount);
    
    if(mouseCounter) {
      if(--mouseCounter == 0) {
        SetCursor(NULL);
      }
    }    
    return TRUE;
  }
  return FALSE;
}

void RunMessageLoop()
{
  BOOL idle = TRUE;
  LONG idleCount = 0;
  MSG msg;
  
  for(;;) {
    while(idle && !::PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE)) {
      if(!OnIdle(idleCount++))
        idle = FALSE;
    }
    
    do {
      if(!::GetMessage(&msg, NULL, NULL, NULL)) {
        // WM_QUIT received
        return;
      }

      if(!Wnd::WalkPreTranslateTree(hWindow, &msg) &&
         !TranslateAccelerator(hWindow, hAccel, &msg)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }

      if(IsIdleMessage(&msg)) {
        idle = TRUE;
        idleCount = 0;
      }
    } while(::PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE));
  }
}

BOOL PASCAL
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
  getScreenSaverEnable();
  
  if(!initApp(hInstance, nCmdShow)) {
    return FALSE;
  }

  if(lpCmdLine != NULL) {
    if(strlen(lpCmdLine) > 0) {
      char *p = lpCmdLine;
      int i = 0;
      char c;
      while((c = *p++)) {
        switch(c) {
        case '"':
          while(*p && (*p != '"')) {
            szFile[i++] = *p++;
          }
          break;
        default:
          szFile[i++] = c;
          break;
        }
      }
      szFile[i] = 0;
      strcpy(filename, szFile);
      p = strrchr(filename, '.');
      if(p)
        *p = NULL;
      
      if(fileOpen())
        emulating = TRUE;
      else
        emulating = FALSE;
    }
  }

  RunMessageLoop();

  if(screenSaverDisabled != screenSaverState)
    setScreenSaverEnable(screenSaverState);
  
  for(int i = 0; i < 10; i++) {
    if(recentFiles[i] != NULL) {
      free(recentFiles[i]);
      recentFiles[i] = NULL;
    }
  }
  
  soundPause();
  soundShutdown();
  if(gbRom != NULL || rom != NULL) {
    if(autoSaveLoadCheatList)
      winSaveCheatListDefault();
    writeBatteryFile();
    emuCleanUp();
  }

  if(languageModule != NULL) {
    FreeLibrary(languageModule);
    languageModule = NULL;
  }
  
  releaseAllObjects();

  remoteCleanUp();

  return 0;
}

void winSaveCheatList(char *name)
{
  if(cartridgeType == 0)
    cheatsSaveCheatList(name);
  else
    gbCheatsSaveCheatList(name);
}

void winSaveCheatListDefault()
{
  char captureBuffer[2048];

  char *p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  char *capdir = regQueryStringValue("saveDir", NULL);
  if(!capdir)
    capdir = getDirFromFile(filename);

  if(isDriveRoot(capdir))
    sprintf(captureBuffer, "%s%s.clt", capdir, p);
  else
    sprintf(captureBuffer, "%s\\%s.clt", capdir, p);

  winSaveCheatList(captureBuffer);
}

void winSaveCheatList()
{
  OPENFILENAME ofn;
  char captureBuffer[2048];

  char *p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  char *capdir = regQueryStringValue("saveDir", NULL);
  if(!capdir)
    capdir = getDirFromFile(filename);

  if(isDriveRoot(capdir))
    sprintf(captureBuffer, "%s%s.clt", capdir, p);
  else
    sprintf(captureBuffer, "%s\\%s.clt", capdir, p);

  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWindow;
  ofn.lpstrFile = captureBuffer;
  ofn.nMaxFile = sizeof(captureBuffer);
  ofn.lpstrFilter =  winLoadFilter(IDS_FILTER_CHEAT_LIST);
  ofn.nFilterIndex = 0; 
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = (const char *)capdir;
  ofn.lpstrTitle = winResLoadString(IDS_SELECT_CHEAT_LIST_NAME);
  ofn.Flags = OFN_PATHMUSTEXIST;

  if(videoOption == VIDEO_320x240) {
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG);
    ofn.Flags |= OFN_ENABLETEMPLATE;
  }
  
  if(GetSaveFileName(&ofn) == FALSE) {
    DWORD res = CommDlgExtendedError();
    return;
  }

  winSaveCheatList(captureBuffer);
}

void winLoadCheatList(char *name)
{
  bool res = false;

  if(cartridgeType == 0)
    res = cheatsLoadCheatList(name);
  else
    res = gbCheatsLoadCheatList(name);

  if(res)
    systemScreenMessage((char *)winResLoadString(IDS_LOADED_CHEATS));
}

void winLoadCheatListDefault()
{
  char captureBuffer[2048];

  char *p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  char *capdir = regQueryStringValue("saveDir", NULL);
  if(!capdir)
    capdir = getDirFromFile(filename);

  if(isDriveRoot(capdir))
    sprintf(captureBuffer, "%s%s.clt", capdir, p);
  else
    sprintf(captureBuffer, "%s\\%s.clt", capdir, p);

  winLoadCheatList(captureBuffer);
}

void winLoadCheatList()
{
  OPENFILENAME ofn;
  char captureBuffer[2048];

  char *p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  char *capdir = regQueryStringValue("saveDir", NULL);
  if(!capdir)
    capdir = getDirFromFile(filename);

  if(isDriveRoot(capdir))
    sprintf(captureBuffer, "%s%s.clt", capdir, p);
  else
    sprintf(captureBuffer, "%s\\%s.clt", capdir, p);

  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWindow;
  ofn.lpstrFile = captureBuffer;
  ofn.nMaxFile = sizeof(captureBuffer);
  ofn.lpstrFilter =  winLoadFilter(IDS_FILTER_CHEAT_LIST);
  ofn.nFilterIndex = 0; 
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = (const char *)capdir;
  ofn.lpstrTitle = winResLoadString(IDS_SELECT_CHEAT_LIST_NAME);
  ofn.Flags = OFN_PATHMUSTEXIST;

  if(videoOption == VIDEO_320x240) {
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG);
    ofn.Flags |= OFN_ENABLETEMPLATE;
  }
  
  if(GetOpenFileName(&ofn) == FALSE) {
    DWORD res = CommDlgExtendedError();
    return;
  }

  winLoadCheatList(captureBuffer);
}

void fileSoundRecord()
{
  char captureBuffer[2048];
  captureBuffer[0] = 0;
  OPENFILENAME ofn;
  char *capdir = regQueryStringValue("soundRecordDir", NULL);
  
  if(!capdir)
    capdir = getDirFromFile(filename);

  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hWindow;
  ofn.lpstrFile = captureBuffer;
  ofn.nMaxFile = sizeof(captureBuffer);
  ofn.lpstrFilter =  winLoadFilter(IDS_FILTER_WAV);
  ofn.nFilterIndex = 0; //selectedFileIndex;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrDefExt = "WAV";  
  ofn.lpstrInitialDir = (const char *)capdir;
  ofn.lpstrTitle = winResLoadString(IDS_SELECT_WAV_NAME);
  ofn.Flags = OFN_PATHMUSTEXIST;

  if(videoOption == VIDEO_320x240) {
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENDLG);
    ofn.Flags |= OFN_ENABLETEMPLATE;
  }
  
  if(GetSaveFileName(&ofn) == FALSE) {
    DWORD res = CommDlgExtendedError();
    return;
  }

  strcpy(soundRecordName, captureBuffer);
  soundRecording = TRUE;
  
  if(ofn.nFileOffset > 0) {
    captureBuffer[ofn.nFileOffset] = 0;
  }

  int len = strlen((const char *)captureBuffer);
  if(len > 3 && captureBuffer[len-1] == '\\')
    captureBuffer[len-1] = 0;
  regSetStringValue("soundRecordDir", captureBuffer);
}

void screenCapture()
{
  char captureBuffer[2048];
  
  char *p = strrchr(filename,'\\');
  if(p)
    p++;
  else
    p = filename;
  char *capdir = regQueryStringValue("captureDir", NULL);
  if(!capdir)
    capdir = getDirFromFile(filename);

  char *ext = "png";

  if(captureFormat != 0)
    ext = "bmp";
  
  if(isDriveRoot(capdir))
    sprintf(captureBuffer, "%s%s.%s", capdir, p, ext);
  else
    sprintf(captureBuffer, "%s\\%s.%s", capdir, p, ext);

  char *exts[] = {".png", ".bmp" };

  FileDlg dlg(hWindow,
              (char *)captureBuffer,
              (int)sizeof(captureBuffer),
              (char *)winLoadFilter(IDS_FILTER_PNG),
              captureFormat ? 2 : 1,
              captureFormat ? "BMP" : "PNG",
              exts,
              (char *)capdir,
              (char *)winResLoadString(IDS_SELECT_CAPTURE_NAME),
              TRUE);

  BOOL res = dlg.DoModal();  
  if(res == FALSE) {
    DWORD res = CommDlgExtendedError();
    return;
  }
  
  if(dlg.getFilterIndex() == 2)
    emuWriteBMP(captureBuffer);
  else
    emuWritePNG(captureBuffer);

  systemScreenMessage((char *)winResLoadString(IDS_SCREEN_CAPTURE));
}

void systemScreenCapture(int captureNumber)
{
  char buffer[2048];
  
  char *captureDir = regQueryStringValue("captureDir", NULL);
  char *p = strrchr(filename,'\\');

  if(p)
    p++;
  else
    p = filename;
  
  if(captureDir == NULL)
    captureDir = getDirFromFile(filename);
  char *ext = "png";
  if(captureFormat != 0)
    ext = "bmp";

  if(isDriveRoot(captureDir))
    sprintf(buffer,
            "%s%s_%02d.%s",
            captureDir,
            p,
            captureNumber,
            ext);
  else
    sprintf(buffer,
            "%s\\%s_%02d.%s",
            captureDir,
            p,
            captureNumber,
            ext);

  if(captureFormat == 0)
    emuWritePNG(buffer);
  else
    emuWriteBMP(buffer);

  systemScreenMessage((char *)winResLoadString(IDS_SCREEN_CAPTURE));
}

u32 systemGetClock()
{
  return GetTickCount();
}

void systemMessage(int number, char *defaultMsg, ...)
{
  char buffer[2048];
  va_list valist;
  const char *msg = (const char *)defaultMsg;
  if(number)
    msg = winResLoadString(number);
  
  va_start(valist, defaultMsg);
  vsprintf(buffer, msg, valist);
  
  MessageBox(GetTopWindow(hWindow), buffer, winResLoadString(IDS_ERROR),
             MB_OK | MB_ICONERROR);
  va_end(valist);
}

void systemSetTitle(char *title)
{
  if(hWindow != NULL) {
    SetWindowText(hWindow, title);
  }
}

void systemScreenMessage(char *msg)
{
  screenMessage = true;
  screenMessageTime = GetTickCount();
  if(strlen(msg) > 40) {
    strncpy(screenMessageBuffer, msg, 40);
    screenMessageBuffer[40] = 0;
  } else
    strcpy(screenMessageBuffer, msg);
}

void systemUpdateMotionSensor()
{
  if(checkKey(motion[KEY_LEFT])) {
    sensorX += 3;
    if(sensorX > 2197)
      sensorX = 2197;
    if(sensorX < 2047)
      sensorX = 2057;
  } else if(checkKey(motion[KEY_RIGHT])) {
    sensorX -= 3;
    if(sensorX < 1897)
      sensorX = 1897;
    if(sensorX > 2047)
      sensorX = 2037;
  } else if(sensorX > 2047) {
    sensorX -= 2;
    if(sensorX < 2047)
      sensorX = 2047;
  } else {
    sensorX += 2;
    if(sensorX > 2047)
      sensorX = 2047;
  }

  if(checkKey(motion[KEY_UP])) {
    sensorY += 3;
    if(sensorY > 2197)
      sensorY = 2197;
    if(sensorY < 2047)
      sensorY = 2057;
  } else if(checkKey(motion[KEY_DOWN])) {
    sensorY -= 3;
    if(sensorY < 1897)
      sensorY = 1897;
    if(sensorY > 2047)
      sensorY = 2037;
  } else if(sensorY > 2047) {
    sensorY -= 2;
    if(sensorY < 2047)
      sensorY = 2047;
  } else {
    sensorY += 2;
    if(sensorY > 2047)
      sensorY = 2047;
  }  
}

int systemGetSensorX()
{
  return sensorX;
}

int systemGetSensorY()
{
  return sensorY;
}

bool systemSoundInit()
{
  HRESULT hr;

  dsoundDLL = LoadLibrary("DSOUND.DLL");
  HRESULT (WINAPI *DSoundCreate)(LPCGUID,LPDIRECTSOUND *,IUnknown *);  
  if(dinputDLL != NULL) {    
    DSoundCreate = (HRESULT (WINAPI *)(LPCGUID,LPDIRECTSOUND *,IUnknown *))
      GetProcAddress(dsoundDLL, "DirectSoundCreate");
    
    if(DSoundCreate == NULL) {
      directXMessage("DirectSoundCreate");
      return FALSE;
    }
  } else {
    directXMessage("DSOUND.DLL");
    return FALSE;
  }
  
  if((hr = DSoundCreate(NULL,&pDirectSound,NULL) != DS_OK)) {
    //    errorMessage(myLoadString(IDS_ERROR_SOUND_CREATE), hr);
    systemMessage(IDS_CANNOT_CREATE_DIRECTSOUND,
                  "Cannot create DirectSound %08x", hr);
    pDirectSound = NULL;
    dsbSecondary = NULL;
    return true;
  }

  if((hr=pDirectSound->SetCooperativeLevel(hWindow,
                                           DSSCL_EXCLUSIVE)) != DS_OK) {
    //    errorMessage(myLoadString(IDS_ERROR_SOUND_LEVEL), hr);
    systemMessage(IDS_CANNOT_SETCOOPERATIVELEVEL,
                  "Cannot SetCooperativeLevel %08x", hr);
    return false;
  }

  DSBUFFERDESC dsbdesc;
  ZeroMemory(&dsbdesc,sizeof(DSBUFFERDESC));
  dsbdesc.dwSize=sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
  
  if((hr=pDirectSound->CreateSoundBuffer(&dsbdesc,
                                         &dsbPrimary,
                                         NULL) != DS_OK)) {
    //    errorMessage(myLoadString(IDS_ERROR_SOUND_BUFFER),hr);
    systemMessage(IDS_CANNOT_CREATESOUNDBUFFER,
                  "Cannot CreateSoundBuffer %08x", hr);
    return false;
  }
  
  // Set primary buffer format

  memset(&wfx, 0, sizeof(WAVEFORMATEX)); 
  wfx.wFormatTag = WAVE_FORMAT_PCM; 
  wfx.nChannels = 2;
  switch(soundQuality) {
  case 2:
    wfx.nSamplesPerSec = 22050;
    soundBufferLen = 736*2;
    soundBufferTotalLen = 7360*2;
    break;
  case 4:
    wfx.nSamplesPerSec = 11025;
    soundBufferLen = 368*2;
    soundBufferTotalLen = 3680*2;
    break;
  default:
    soundQuality = 1;
    wfx.nSamplesPerSec = 44100;
    soundBufferLen = 1470*2;
    soundBufferTotalLen = 14700*2;
  }
  wfx.wBitsPerSample = 16; 
  wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

  if((hr = dsbPrimary->SetFormat(&wfx)) != DS_OK) {
    //    errorMessage(myLoadString(IDS_ERROR_SOUND_PRIMARY),hr);
    systemMessage(IDS_CANNOT_SETFORMAT_PRIMARY,
                  "Cannot SetFormat for primary %08x", hr);
    return false;
  }
  
  ZeroMemory(&dsbdesc,sizeof(DSBUFFERDESC));  
  dsbdesc.dwSize = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_CTRLPOSITIONNOTIFY|DSBCAPS_CTRLVOLUME;
  dsbdesc.dwBufferBytes = soundBufferTotalLen;
  dsbdesc.lpwfxFormat = &wfx;

  if((hr = pDirectSound->CreateSoundBuffer(&dsbdesc, &dsbSecondary, NULL))
     != DS_OK) {
    dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
    if((hr = pDirectSound->CreateSoundBuffer(&dsbdesc, &dsbSecondary, NULL))
       != DS_OK) {
      systemMessage(IDS_CANNOT_CREATESOUNDBUFFER_SEC,
                    "Cannot CreateSoundBuffer secondary %08x", hr);
      return false;
    }
  }

  dsbSecondary->SetCurrentPosition(0);

  if(!useOldSync) {
    hr = dsbSecondary->QueryInterface(IID_IDirectSoundNotify,
                                      (void **)&dsbNotify);
    if(!FAILED(hr)) {
      dsbEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
      
      DSBPOSITIONNOTIFY notify[10];
      
      for(int i = 0; i < 10; i++) {
        notify[i].dwOffset = i*soundBufferLen;
        notify[i].hEventNotify = dsbEvent;
      }
      if(FAILED(dsbNotify->SetNotificationPositions(10, notify))) {
        dsbNotify->Release();
        dsbNotify = NULL;
        CloseHandle(dsbEvent);
        dsbEvent = NULL;
      }
    }
  }
  
  hr = dsbPrimary->Play(0,0,DSBPLAY_LOOPING);

  if(hr != DS_OK) {
    //    errorMessage(myLoadString(IDS_ERROR_SOUND_PLAYPRIM), hr);
    systemMessage(IDS_CANNOT_PLAY_PRIMARY, "Cannot Play primary %08x", hr);
    return false;
  }

  systemSoundOn = true;
  
  return true;
}


void systemSoundShutdown()
{
  if(soundRecording) {
    if(soundRecorder != NULL) {
      delete soundRecorder;
      soundRecorder = NULL;
    }
    soundRecording = FALSE;
  }
  
  if(dsbNotify != NULL) {
    dsbNotify->Release();
    dsbNotify = NULL;
  }

  if(dsbEvent != NULL) {
    CloseHandle(dsbEvent);
    dsbEvent = NULL;
  }
  
  if(pDirectSound != NULL) {
    if(dsbPrimary != NULL) {
      dsbPrimary->Release();
      dsbPrimary = NULL;
    }
    
    if(dsbSecondary != NULL) {
      dsbSecondary->Release();
      dsbSecondary = NULL;
    }
    
    pDirectSound->Release();
    pDirectSound = NULL;
  }
  
  if(dsoundDLL != NULL) {
    FreeLibrary(dsoundDLL);
    dsoundDLL = NULL;
  }
}

void systemSoundPause()
{
  if(dsbSecondary != NULL) {
    DWORD status = 0;
    dsbSecondary->GetStatus(&status);
    
    if(status & DSBSTATUS_PLAYING) {
      dsbSecondary->Stop();
    }
  }  
}

void systemSoundReset()
{
  if(dsbSecondary) {
    dsbSecondary->Stop();
    dsbSecondary->SetCurrentPosition(0);
  }  
}

void systemSoundResume()
{
  if(dsbSecondary != NULL) {
    dsbSecondary->Play(0,0,DSBPLAY_LOOPING);
  }  
}

void systemWriteDataToSoundBuffer()
{
  int len = soundBufferLen;
  LPVOID  lpvPtr1; 
  DWORD   dwBytes1; 
  LPVOID  lpvPtr2; 
  DWORD   dwBytes2; 

  if(!pDirectSound)
    return;

  LONG volume = 0;

  dsbPrimary->GetVolume(&volume);

  if(soundRecording) {
    if(dsbSecondary) {
      if(soundRecorder == NULL) {
        soundRecorder = new CWaveSoundWrite;
        WAVEFORMATEX format;
        dsbSecondary->GetFormat(&format, sizeof(format), NULL);
        soundRecorder->Open(soundRecordName, &format);
      }
    }
      
    if(soundRecorder) {
      UINT wrote = 0;
      soundRecorder->Write(len, (u8 *)soundFinalWave, &wrote);
    }
  }
  
  HRESULT hr;
  if(!speedup && synchronize) {
    DWORD status=0;
    hr = dsbSecondary->GetStatus(&status);
    if(status && DSBSTATUS_PLAYING) {
      if(!soundPaused) {      
        DWORD play;
        
        while(true) {
          dsbSecondary->GetCurrentPosition(&play, NULL);
          
          if(play < soundNextPosition ||
             play > soundNextPosition+soundBufferLen) {
            break;
          }

          if(dsbEvent)
            WaitForSingleObject(dsbEvent, 50);
        }
      }
    } else {
      soundPaused = 1;
    }
  }
  // Obtain memory address of write block. This will be in two parts
  // if the block wraps around.
  hr = dsbSecondary->Lock(soundNextPosition, soundBufferLen, &lpvPtr1, 
                          &dwBytes1, &lpvPtr2, &dwBytes2,
                          0);
  
  // If DSERR_BUFFERLOST is returned, restore and retry lock. 
  if (DSERR_BUFFERLOST == hr) { 
    dsbSecondary->Restore(); 
    hr = dsbSecondary->Lock(soundNextPosition, soundBufferLen,&lpvPtr1,
                            &dwBytes1, &lpvPtr2, &dwBytes2,
                            0);
  } 

  soundNextPosition += soundBufferLen;
  soundNextPosition = soundNextPosition % soundBufferTotalLen;
  
  if SUCCEEDED(hr) { 
    // Write to pointers. 
    CopyMemory(lpvPtr1, soundFinalWave, dwBytes1); 
    if (NULL != lpvPtr2) { 
      CopyMemory(lpvPtr2, soundFinalWave+dwBytes1, dwBytes2); 
    } 
    // Release the data back to DirectSound. 
    hr = dsbSecondary->Unlock(lpvPtr1, dwBytes1, lpvPtr2, 
                              dwBytes2);
  }
}

bool systemCanChangeSoundQuality()
{
  return true;
}

bool systemPauseOnFrame()
{
  if(winPauseNextFrame) {
    paused = TRUE;
    winPauseNextFrame = FALSE;
    return true;
  }
  return false;
}

void winCenterWindow(HWND window)
{
  RECT parentRect;
  RECT rect;
  HWND parent = GetParent(window);

  if(parent == NULL)
    parent = GetDesktopWindow();

  GetWindowRect(parent, &parentRect);
  GetWindowRect(window, &rect);

  int x = parentRect.left + (parentRect.right - parentRect.left -
                             rect.right + rect.left) / 2;
  int y = parentRect.top + (parentRect.bottom - parentRect.top -
                            rect.bottom + rect.top) / 2;

  if(x < 0)
    x = 0;
  if(y < 0)
    y = 0;
  
  SetWindowPos(window, NULL, x, y, -1, -1,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);  
}

void winAddUpdateListener(IUpdateListener *l)
{
  updateList.push_back(l);
}

void winRemoveUpdateListener(IUpdateListener *l)
{
  updateList.remove(l);
}

extern void toolsLog(char *);

void log(char *defaultMsg, ...)
{
  char buffer[2048];
  va_list valist;
  
  va_start(valist, defaultMsg);
  vsprintf(buffer, defaultMsg, valist);

  toolsLog(buffer);
  
  va_end(valist);
}

void winSignal(int, int)
{
}

#define CPUReadByteQuick(addr) \
  map[(addr)>>24].address[(addr) & map[(addr)>>24].mask]

void winOutput(char *s, u32 addr)
{
  if(s)
    toolsLog(s);
  else {
    CStdString str = "";
    
    char c;

    c = CPUReadByteQuick(addr);
    addr++;
    while(c) {
      str += c;
      c = CPUReadByteQuick(addr);
      addr++;
    }
    toolsLog((char *)(LPCSTR)str);
  }  
}
