CC=gcc
PREFIX=/usr

CFILES=$(wildcard src/*.c)
HFILES=$(wildcard src/*.h)
LIBFILES=$(wildcard src/lib/**/*.c)
INCLUDE=-Iinclude/
CFLAGS=-Llib/ -l:discord_game_sdk.so -lpthread -lX11 -lcurl -lpcre2-8 -lm
CFLAGS_OFFLINE=-Llib/ -l:discord_game_sdk.so -lpthread -lX11 -lpcre2-8 -lm -DOFFLINE

APPNAME=rpcfetch

TARGET=build/$(APPNAME)

all: $(TARGET)

$(TARGET): $(CFILES) $(HFILES)
	mkdir -p build
	$(CC) $(INCLUDE) $(CFILES) $(LIBFILES) $(CFLAGS) -o $@

offline: CFLAGS := $(CFLAGS_OFFLINE)
offline: $(TARGET)

clean:
	rm -rf build

install: $(TARGET)
	mkdir -p ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}${PREFIX}/lib
	cp -f $(TARGET) ${DESTDIR}${PREFIX}/bin
	cp -f lib/discord_game_sdk.so ${DESTDIR}${PREFIX}/lib
	chmod 755 ${DESTDIR}${PREFIX}/bin/$(APPNAME)

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/$(APPNAME)

.PHONY: clean install uninstall
