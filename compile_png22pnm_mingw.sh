#! /bin/sh
# by pts@fazekas.hu at Tue Jan 24 20:40:47 CET 2017

set -ex

# config.h is not needed by png22pnm.
# -DNO_VIZ is needed to avoid dllexport visibility warnings.
i586-mingw32msvc-gcc -DNO_VIZ -s -O3 -Ixstaticdeps -ffunction-sections -fdata-sections -Wl,--gc-sections -ansi -pedantic -Wall -W -Wbad-function-cast -Wmissing-declarations -Wstrict-prototypes -Wpointer-arith -Wcast-align -Winline -Wmissing-prototypes png22pnm.c xstaticdeps/pngall.c xstaticdeps/zall.c -lm -o png22pnm.uncompressed.exe
cp -a png22pnm.uncompressed.exe png22pnm.exe
upx.pts --brute png22pnm.exe

: OK.
