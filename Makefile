TARGET = CitroPSP
OBJS = main.o game.o gfx.o render.o input.o audio.o data.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lpspgu -lpspgum -lpspge -lpspdisplay -lpspctrl -lpspaudio -lpspaudiolib -lm
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = La Nuit des Citrouilles
# Optionnel : ICON0.png / PIC1.png dans le même dossier
# PSP_EBOOT_ICON = ICON0.png

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
