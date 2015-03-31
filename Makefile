TARGET := 3ds_test
OBJS :=
OBJS += test.o main.o
#NO_SMDH = 1


CTRULIB = ../ctrulib/libctru
AEMSTRO = ../aemstro

export PATH	:=	$(DEVKITARM)/bin:$(PATH)

INCDIRS := -I$(CTRULIB)/include
LIBDIRS := -L$(CTRULIB)/lib


ARCH     := -march=armv6k -mtune=mpcore -mfloat-abi=hard

CFLAGS	:=	-g -Wall -O2 -mword-relocations \
			-fomit-frame-pointer -ffast-math \
			$(ARCH)

CFLAGS	+=	-DARM11 -D_3DS

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS   =	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lctru -lm

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(TARGET)

clean:
	rm -f $(OBJS)
	rm -f $(TARGET).3dsx
	rm -f $(TARGET).elf


$(TARGET): $(TARGET).3dsx
$(TARGET).3dsx	:	$(TARGET).elf
$(TARGET).elf	:	$(OBJS)

ifeq ($(strip $(APP_TITLE)),)
APP_TITLE	:=	$(notdir $(TARGET))
endif

ifeq ($(strip $(APP_DESCRIPTION)),)
APP_DESCRIPTION	:=	Built with devkitARM & libctru
endif

ifeq ($(strip $(APP_AUTHOR)),)
APP_AUTHOR	:=	Unspecified Author
endif

ifeq ($(strip $(APP_ICON)),)
APP_ICON	:=	$(CTRULIB)/default_icon.png
endif

PREFIX		:=	$(DEVKITARM)/bin/arm-none-eabi-

CC      := $(PREFIX)gcc
CXX     := $(PREFIX)g++
AS      := $(PREFIX)as
AR      := $(PREFIX)ar
OBJCOPY := $(PREFIX)objcopy
STRIP   := $(PREFIX)strip
NM      := $(PREFIX)nm
LD      := $(CXX)


%.o: %.vsh
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	python $(AEMSTRO)/aemstro_as.py $< $(notdir $<).shbin
	bin2s $(notdir $<).shbin | $(PREFIX)as -o $@
	echo "extern const u8" `(echo $(notdir $<).shbin | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(notdir $<).shbin | tr . _)`.h
	echo "extern const u8" `(echo $(notdir $<).shbin | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(notdir $<).shbin | tr . _)`.h
	echo "extern const u32" `(echo $(notdir $<).shbin | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(notdir $<).shbin | tr . _)`.h
	rm $(notdir $<).shbin


%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(INCDIRS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(INCDIRS)

%.o: %.s
	$(CC) -c -o $@ $< $(ASFLAGS)

%.o: %.S
	$(CC) -c -o $@ $< $(ASFLAGS)

%.a:
	$(AR) -rc $@ $^

%.vsh:

#---------------------------------------------------------------------------------
%.smdh: $(APP_ICON) $(MAKEFILE_LIST)
	@echo building ... $(notdir $@)
	smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $(APP_ICON) $@

#---------------------------------------------------------------------------------
%.3dsx: %.elf
	@echo building ... $(notdir $@)
	3dsxtool $< $@ $(_3DSXFLAGS)

#---------------------------------------------------------------------------------
%.elf:
	@echo linking $(notdir $@)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBDIRS) $(LIBS) -o $@
	$(NM) -CSn $@ > $(notdir $*.lst)

