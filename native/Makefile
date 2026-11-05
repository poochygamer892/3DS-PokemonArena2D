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
TEX3DS       ?= tex3ds
APP_TITLE    := Pokemon Arena 2D
APP_DESCRIPTION := Poke Arena 2D homebrew adaptation
APP_AUTHOR   := poochygamer892
ICON         := icon.png

ARCH         := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS       := -g -Wall -Wextra -O2 -mword-relocations -ffunction-sections -fdata-sections $(ARCH) $(INCLUDE) -D__3DS__
LDFLAGS      := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(TARGET).map -Wl,--gc-sections
LIBS         := -lcitro2d -lcitro3d -lctru -lm
LIBDIRS      := $(CTRULIB)

.PHONY: all clean assets $(BUILD)
.NOTPARALLEL: all

ifeq ($(BUILD),$(notdir $(CURDIR)))

$(OUTPUT).3dsx: $(OUTPUT).elf $(_3DSXDEPS)

$(OUTPUT).elf: $(OFILES)

else

export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export APP_ICON := $(CURDIR)/$(ICON)
export _3DSXDEPS := $(if $(NO_SMDH),,$(OUTPUT).smdh)
ifeq ($(strip $(NO_SMDH)),)
export _3DSXFLAGS += --smdh=$(OUTPUT).smdh
endif
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export OFILES_SOURCES := $(CFILES:.c=.o)
export OFILES := $(OFILES_SOURCES)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export LD := $(CC)

all: assets $(BUILD)

# Battle sprites retain 75% of source detail in a nearest-filtered RGBA4 atlas.
# Opaque arena images use RGB565 to preserve gradients without extra VRAM.
assets:
	@mkdir -p generated/pokearena2d include
	@python3 scripts/prepare_poke2d_assets.py \
		--source assets/pokearena2d/source \
		--output generated/pokearena2d \
		--include include/poke2d_sprite_meta.h
	@$(TEX3DS) --atlas -f rgb565 -z lz11 \
		-H include/poke2d_background_indices.h \
		-o generated/pokearena2d/poke2d_background_atlas.t3x \
		$$(find generated/pokearena2d/background_input -maxdepth 1 -name '*.png' -type f | sort)
	@$(TEX3DS) --atlas -f rgba4 -z lz11 \
		-H include/poke2d_sprite_indices.h \
		-o generated/pokearena2d/poke2d_atlas.t3x \
		$$(find generated/pokearena2d/atlas_input -maxdepth 1 -name '*.png' -type f | sort)

$(BUILD): assets
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -rf $(BUILD) $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh $(TARGET).map

endif
