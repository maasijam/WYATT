# Project Name
TARGET = grainstorm
USE_DAISYSP_LGPL=1

USE_FATFS = 1

APP_TYPE = BOOT_SRAM

LDSCRIPT=daisy.lds

# Library Locations
LIBDAISY_DIR = ../../libdaisy
DAISYSP_DIR = ../../DaisySP


# Sources
CPP_SOURCES = grainstorm.cpp ../../common/osc_lfo.cpp ../../Tft/dma2d.cpp ../../Tft/ili9341_ui_driver.cpp ../../daisy_wyatt.cpp ../../libDaisyCustom/encoderNoSwitch.cpp settings.cpp ../../common/tft_lib.cpp 


C_INCLUDES += -Wno-unused-local-typedefs
OPT = -Os -s

# Core location, and generic makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile