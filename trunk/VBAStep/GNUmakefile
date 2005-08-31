include $(GNUSTEP_MAKEFILES)/common.make

APP_NAME = VBAStep

VBAStep_HEADERS = MainController.h
VBAStep_OBJC_FILES = \
	main.m \
	MainController.m \
	System.m Emulator.m \
	GbaScreen.m \


VBAStep_RESOURCE_FILES = \
	 MainMenu-GNUstep.gsmarkup \
	 MainMenu-OSX.gsmarkup \
	 MainWindow.gsmarkup \


ADDITIONAL_TOOL_LIBS +=  -lgba -lprof -lpng -lz
ADDITIONAL_LIB_DIRS += -L.
ADDITIONAL_INCLUDE_DIRS += -I../VisualBoyAdvance/src

ifeq ($(FOUNDATION_LIB), apple)
#  ADDITIONAL_INCLUDE_DIRS += -framework Renaissance
  ADDITIONAL_GUI_LIBS += -framework Renaissance
else
  ADDITIONAL_GUI_LIBS += -lRenaissance
endif

LD = g++

include $(GNUSTEP_MAKEFILES)/application.make


