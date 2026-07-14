# For Linux build:
# 	make
#
# For windows build:
# 	make TARGET=windows
#
# Build options:
# 	make BUILD=release
# 	make BUILD=debug

CC ?= gcc

TARGET ?= linux

BUILD ?= release

USE_GNU_READL ?= 0

LIBS   = -lm -ldl
SRCDIR = src
OUT = seal

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:.c=.o)

LDFLAGS = -rdynamic
DEFS   =

ifeq ($(BUILD),release)
	CFLAGS = -O2
else
	CFLAGS = -O0 -g
endif

ifeq ($(TARGET),windows)
	CC = x86_64-w64-mingw32-gcc
	LIBS = -lm
	OUT = seal.exe
	LDFLAGS = # -Wl,--export-all-symbols -Wl,--out-implib,libseal.dll.a
	DEFS += -DSEAL_BUILDING_MAIN
endif

ifeq ($(USE_GNU_READL),1)
	ifeq ($(TARGET),linux)
		DEFS += -DUSE_GNU_READL=1
		LIBS += -lreadline
	endif
endif

all: $(OUT)

$(OUT): $(OBJS)
	$(CC) $(OBJS) -o $(OUT) $(LDFLAGS) $(LIBS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(DEFS) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(OUT)

install: $(OUT)
	cp $(OUT) /usr/local/bin/
	mkdir -p /usr/local/include/seal
	cp $(SRCDIR)/seal.h $(SRCDIR)/sealconf.h /usr/local/include/seal/

uninstall:
	rm -f /usr/local/bin/$(OUT)
	rm -rf /usr/local/include/seal

.PHONY: all clean install uninstall
