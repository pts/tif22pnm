#! /bin/sh
# by pts@fazekas.hu at Mon Jul 24 17:18:38 CEST 2017

set -ex

# $ docker image ls --digests multiarch/crossbuild
# The image ID is also a digest, and is a computed SHA256 hash of the image configuration object, which contains the digests of the layers that contribute to the image's filesystem definition.
# REPOSITORY             TAG                 DIGEST                                                                    IMAGE ID            CREATED             SIZE
# multiarch/crossbuild   latest              sha256:84a53371f554a3b3d321c9d1dfd485b8748ad6f378ab1ebed603fe1ff01f7b4d   846ea4d99d1a        5 months ago        2.99 GB
   CC="docker run -t -v $PWD:/workdir multiarch/crossbuild /usr/osxcross/bin/o32-clang -mmacosx-version-min=10.5"
  CXX="docker run -t -v $PWD:/workdir multiarch/crossbuild /usr/osxcross/bin/o32-clang++ -mmacosx-version-min=10.5"
 CCLD="docker run -t -v $PWD:/workdir multiarch/crossbuild /usr/osxcross/bin/o32-clang -mmacosx-version-min=10.5 -Ldarwin_libgcc/i386-apple-darwin10/4.9.4 -lSystem -lgcc -lcrt1.10.5.o -nostdlib"
STRIP="docker run -t -v $PWD:/workdir multiarch/crossbuild /usr/osxcross/bin/i386-apple-darwin14-strip"
test -f darwin_libgcc/i386-apple-darwin10/4.9.4/libgcc.a

CFLAGS="-c -O3 -Ixstaticdeps -ffunction-sections -fdata-sections -ansi -pedantic -Wall -W -Wbad-function-cast -Wmissing-declarations -Wstrict-prototypes -Wpointer-arith -Wcast-align -Winline -Wmissing-prototypes"
# config.h is not needed by png22pnm, thus configure not run here.
rm -f *.o xstaticdeps/*.o
$CC $CFLAGS xstaticdeps/zall.c
$CC $CFLAGS -Wno-self-assign xstaticdeps/pngall.c
$CC $CFLAGS png22pnm.c
$CCLD -Wl,-dead_strip -o png22pnm.darwinc32 *.o
$STRIP png22pnm.darwinc32

: OK.
