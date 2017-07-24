#! /bin/sh
# by pts@fazekas.hu at Mon Jul 24 17:18:38 CEST 2017

set -ex

# $ docker image ls --digests multiarch/crossbuild
# The image ID is also a digest, and is a computed SHA256 hash of the image configuration object, which contains the digests of the layers that contribute to the image's filesystem definition.
# REPOSITORY             TAG                 DIGEST                                                                    IMAGE ID            CREATED             SIZE
# multiarch/crossbuild   latest              sha256:84a53371f554a3b3d321c9d1dfd485b8748ad6f378ab1ebed603fe1ff01f7b4d   846ea4d99d1a        5 months ago        2.99 GB
  CCC="docker run -t -v $PWD:/workdir multiarch/crossbuild /usr/osxcross/bin/o64-clang -mmacosx-version-min=10.5 -c"
 CXXC="docker run -t -v $PWD:/workdir multiarch/crossbuild /usr/osxcross/bin/o64-clang++ -mmacosx-version-min=10.5 -c"
 CCLD="docker run -t -v $PWD:/workdir multiarch/crossbuild /usr/osxcross/bin/o64-clang -mmacosx-version-min=10.5 -Ldarwin_libgcc/x86_64-apple-darwin10/4.9.4 -lSystem -lgcc -lcrt1.10.5.o -nostdlib"
STRIP="docker run -t -v $PWD:/workdir multiarch/crossbuild /usr/osxcross/bin/x86_64-apple-darwin14-strip"
test -f darwin_libgcc/x86_64-apple-darwin10/4.9.4/libgcc.a

CFLAGS="-O3 -Ixstaticdeps -ffunction-sections -fdata-sections -ansi -pedantic -Wall -W -Wbad-function-cast -Wmissing-declarations -Wstrict-prototypes -Wpointer-arith -Wcast-align -Winline -Wmissing-prototypes"
# config.h is not needed by png22pnm, thus configure not run here.
# -Wno-self-assign needed by xstaticdeps/pngall.c
$CCLD $CFLAGS -Wl,-dead_strip -o png22pnm.darwinc64 -Wno-self-assign xstaticdeps/zall.c xstaticdeps/pngall.c png22pnm.c
$STRIP png22pnm.darwinc64

: OK.
