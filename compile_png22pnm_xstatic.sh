#! /bin/sh
# by pts@fazekas.hu at Tue Jan 24 20:40:47 CET 2017

set -ex

# config.h is not needed by png22pnm, thus configure not run here.
# -D_BSD_SOURCE is needed for snprintf.
xstatic gcc -s -O3 -Ixstaticdeps -ffunction-sections -fdata-sections -Wl,--gc-sections -D_BSD_SOURCE -ansi -pedantic -Wall -W -Wbad-function-cast -Wmissing-declarations -Wstrict-prototypes -Wpointer-arith -Wcast-align -Winline -Wmissing-prototypes png22pnm.c xstaticdeps/pngall.c xstaticdeps/zall.c -lm -o png22pnm.xstatic.uncompressed
elfosfix.pl png22pnm.xstatic.uncompressed
cp -a png22pnm.xstatic.uncompressed png22pnm.xstatic
upx.pts --brute png22pnm.xstatic
elfosfix.pl png22pnm.xstatic


: OK.
