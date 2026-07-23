# Project Name
TARGET = grainwaves
USE_DAISYSP_LGPL=1
#RESOURCES      = quaxx/resources
#DEBUG = 1
#LDFLAGS += -u _printf_float
 ##APP_TYPE = BOOT_NONE
USE_FATFS = 1

APP_TYPE = BOOT_SRAM
#LDSCRIPT=sram.lds
LDSCRIPT=daisy.lds

# Library Locations
LIBDAISY_DIR = ../../libdaisy
DAISYSP_DIR = ../../DaisySP
#STMLIB_DIR = ../../stmlib

# Sources
CPP_SOURCES = grainwaves.cpp osc_lfo.cpp ../../Tft/dma2d.cpp ../../Tft/ili9341_ui_driver.cpp ../../daisy_wyatt.cpp ../../libDaisyCustom/encoderNoSwitch.cpp settings.cpp ../../common/SRButton.cpp
#C_INCLUDES =  -Iwreath
#CPP_SOURCES += $(STMLIB_DIR)/dsp/units.cc \
#$(STMLIB_DIR)/utils/random.cc \
# cv_scaler.cc \


# Can't actually add to CFLAGS.. due to libDaisy stuff
# silences warning from stmlib JOIN macros
C_INCLUDES += -Wno-unused-local-typedefs
OPT = -Os -s

# Core location, and generic makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile