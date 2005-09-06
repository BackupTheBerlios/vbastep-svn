include $(GNUSTEP_MAKEFILES)/common.make

APP_NAME = VBAStep

VBAStep_HEADERS = MainController.h
VBAStep_OBJC_FILES = \
	main.m \
	MainController.m \
	System.m Emulator.m \
	GbaScreen.m \
	TileGrid.m \
	ColorTile.m \
	PaletteController.m \
	GdbController.m \
	PCList.m \
	MemLogger.m \

VBAStep_MAIN_MODEL_FILE = MainMenu.nib

VBAStep_RESOURCE_FILES = \
	 MainMenu.nib \
	 MainMenu.gmodel \


ADDITIONAL_TOOL_LIBS +=  -lgba -lprof -lpng -lz
ADDITIONAL_LIB_DIRS += -L.
ADDITIONAL_INCLUDE_DIRS += -I../VisualBoyAdvance/src

LD = g++

include $(GNUSTEP_MAKEFILES)/application.make


