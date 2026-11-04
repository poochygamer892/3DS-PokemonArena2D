# Native Nintendo 3DS build using devkitARM, libctru, citro3d and citro2d.

ifeq ($(strip $(DEVKITARM)),)
$(error Set DEVKITARM to the devkitPro devkitARM directory)
endif

include $(DEVKITARM)/3ds_rules

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
CFLAGS       := -g -Wall -Wextra -O2 -mword-relocations -ffunction-sections -fdata-sections $(ARCH) $(INCLUDE) -D__3DS__
LDFLAGS      := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(TARGET).map -Wl,--gc-sections
LIBS         := -lcitro2d -lcitro3d -lctru -lm
LIBDIRS      := $(CTRULIB)

.PHONY: all clean

ifeq ($(BUILD),$(notdir $(CURDIR)))

$(OUTPUT).3dsx: $(OUTPUT).elf $(_3DSXDEPS)

$(OUTPUT).elf: $(OFILES)

else

export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export OFILES_SOURCES := $(CFILES:.c=.o)
export OFILES := $(OFILES_SOURCES)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export LD := $(CC)

all: $(BUILD)

$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -rf $(BUILD) $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh $(TARGET).map

endif
