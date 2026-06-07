EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = StoreStation
PSP_EBOOT_ICON = assets/ICON0.png
PSP_EBOOT_PSAR = psar.zip

.PHONY: build_and_clean
build_and_clean:
	$(MAKE) all
	rm -f $(OBJS) PARAM.SFO $(TARGET).elf $(TARGET).prx $(PSP_EBOOT_PSAR)

BUILD_PRX = 1
TARGET = Application
OBJS = main.o
INCDIR =
CFLAGS = -Wall -Os -DDEBUG=false -DATTR_VERSION_STR=\"1.3\" -ffast-math -fno-strict-aliasing -Wshadow -Werror
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS= -lpspnet_apctl -lpsphttp -lraylib -lphysfs -lcjson -lz -lglut -lGLU -lGL -lpspfpu -lpspvfpu -lpsppower -lpspaudio -lpspaudiolib -lmad -lpspmp3 -lpspjpeg

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

EBOOT.PBP: $(PSP_EBOOT_PSAR)

$(PSP_EBOOT_PSAR):
	zip -r $(PSP_EBOOT_PSAR) assets/

.PHONY: realclean all
realclean:
	rm -f $(PSP_EBOOT_PSAR) $(EXTRA_TARGETS)
	rm -f $(OBJS) PARAM.SFO $(TARGET).elf $(TARGET).prx