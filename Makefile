BUILD_PRX = 1
TARGET = Application
OBJS = main.o
INCDIR =
CFLAGS = -Wall -o3 -DDEBUG=false -DATTR_VERSION_STR=\"1.0\"
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS= -lpspnet_apctl -lpsphttp -lraylib -lphysfs -lcjson -lz -lglut -lGLU -lGL -lpspfpu -lpspvfpu -lpsppower -lpspaudio -lpspaudiolib -lmad -lpspmp3 -lpspjpeg

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = StoreStation
PSP_EBOOT_ICON = assets/ICON0.png
PSP_EBOOT_PSAR = psar.zip
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

realclean:
	/bin/rm $(PSP_EBOOT_PSAR)
	/bin/zip -r $(PSP_EBOOT_PSAR) assets/
	/bin/rm -f $(OBJS) PARAM.SFO $(TARGET).elf $(TARGET).prx
	ebootsign EBOOT.PBP SIGNED.PBP
	/bin/mv SIGNED.PBP EBOOT.PBP

all: realclean