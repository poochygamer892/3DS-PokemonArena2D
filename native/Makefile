# Native Nintendo 3DS build using devkitARM, libctru, citro3d and citro2d.

ifeq ($(strip $(DEVKITARM)),)
$(error Set DEVKITARM to the devkitPro devkitARM directory)
endif

TARGET       := pokemonarena2d
BUILD        := build
SOURCES      := source
DATA         :=
INCLUDES     := include
APP_TITLE    := Pokemon Arena 2D
APP_DESCRIPTION := Real-time Pokemon battle prototype
APP_AUTHOR   := poochygamer892
ICON         := icon.png

ARCH         := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS       := -g -Wall -Wextra -O2 -mword-relocations -ffunction-sections -fdata-sections $(ARCH) -D__3DS__
LDFLAGS      := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(TARGET).map -Wl,--gc-sections
LIBS         := -lcitro2d -lcitro3d -lctru -lm
LIBDIRS      := $(CTRULIB)

include $(DEVKITARM)/3ds_rules
