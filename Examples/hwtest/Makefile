# Project Name
TARGET = hwtest

USE_FATFS = 1

APP_TYPE = BOOT_NONE


# Library Locations
LIBDAISY_DIR = ../../libdaisy
DAISYSP_DIR = ../../DaisySP


# Sources
CPP_SOURCES = hwtest.cpp ../../Tft/dma2d.cpp ../../Tft/ili9341_ui_driver.cpp ../../daisy_wyatt.cpp ../../libDaisyCustom/encoderNoSwitch.cpp ../../common/tft_lib.cpp


# Can't actually add to CFLAGS.. due to libDaisy stuff
# silences warning from stmlib JOIN macros
C_INCLUDES += -Wno-unused-local-typedefs
OPT = -Os -s

# Core location, and generic makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile