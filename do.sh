#! /bin/bash --
#
# do.sh, formerly compile.sh for reTCP
# by pts@inf.bme.hu at Thu Mar  7 17:48:45 CET 2002
# Sun Mar 31 18:38:11 CEST 2002
# Sun Mar 31 23:51:49 CEST 2002
#

if [ -f tif22pnm.c ]; then : ; else
  echo "$0: tif22pnm.c should be found in current dir"
  exit 4
fi

if [ "$1" != clean ] && [ "$1" != confclean ] && [ "$1" != "" ] && [ "$1" != dist ]; then
  if [ -f cc_help.sh ]; then
    source cc_help.sh
  else
    echo "$0: cc_help.sh doesn't exist."
    echo "Please run ./configure first, it will create cc_help.sh for you."
    exit 3
  fi
fi

if [ "$1" = compile ]; then
  case "$ENABLE_DEBUG" in
    assert) CMD=asserted ;;
    yes) CMD=debug ;;
    checker) CMD=checker ;;
    *) CMD=final ;;
  esac
else
  CMD="$1"
fi

if [ x"$CMD" != x ]; then echo "Executing: $CMD"; fi

if [ "$CMD" = fast ]; then
  L_CC="$CC -s -O3 -DNDEBUG"
  L_LD="$LD -s"
elif [ "$CMD" = debug ]; then
  L_CC="$CC $GFLAG"
  L_LD="$LD"
elif [ "$CMD" = efence ]; then
  L_CC="$CC $GFLAG"
  L_LD="$LD -lefence"
elif [ "$CMD" = checker ]; then
  L_CC="checkergcc $GFLAG"
  L_LD="checkergcc"
elif [ "$CMD" = small ]; then
  L_CC="$CC -Os -DNDEBUG"
  L_LD="$LD -s"
elif [ "$CMD" = final ]; then
  L_CC="$CC -O2 -DNDEBUG"
  L_LD="$LD -s"
elif [ "$CMD" = asserted ]; then
  L_CC="$CC -O2"
  L_LD="$LD -s"
elif [ "$CMD" = clean ]; then
  rm -f *.o core DEADJOE tif22pnm png22pnm
  exit
elif [ "$CMD" = confclean ]; then
  set -ex
  rm -f tif22pnm png22pnm *.o core DEADJOE *~ autom4te.cache/*
  rm -f cc_help.sh config.status config.log config.cache config.h
  rmdir autom4te.cache || true
  if type -p autoconf >/dev/null; then
    rm -f configure
    autoconf
  fi
  exit
elif [ "$CMD" = dist ]; then
  set -ex
  rm -f tif22pnm png22pnm *.o core DEADJOE *~ 
  rm -f cc_help.sh config.status config.log config.cache config.h
  set +ex
  if type -p autoconf >/dev/null; then
    rm -f configure
    autoconf
  fi
  WD="`pwd`"
  UPDIR="${WD%/*}"
  MYDIR="${WD##*/}"
  export MYDIR

  cd "$UPDIR"
  tar czvf "$MYDIR.tar.gz" `<"$MYDIR"/files perl -pi -e '$_="$ENV{MYDIR}/$_"'`
  [ -s "$UPDIR/$MYDIR.tar.gz" ] || exit 4
  echo "Created $UPDIR/$MYDIR.tar.gz" >&2
  exit
else
  echo "Usage: $0 <compile |clean|confclean |final|asserted|fast|debug|checker|small|dist>"
  exit 2
fi

build() {
  [ "$LIBS" = "missing" ] && return 0
  OS=
  for C in $SOURCES; do OS="$OS ${C%.*}.o"; done
  set -e
  for C in $SOURCES; do
    echo + $L_CC $CPPFLAGS $CFLAGS $CFLAGSB -c $C
    $L_CC $CPPFLAGS $CFLAGS $CFLAGSB -c $C
  done
  echo + $L_LD $LDFLAGS $OS -o "$TARGET" $LIBS
  $L_LD $LDFLAGS $OS -o "$TARGET" $LIBS
  set +e
  export TARGET
  echo "Created executable file: $TARGET (size: `perl -e 'print -s $ENV{TARGET}'`)."
}

SOURCES='ptspnm.c minigimp.c miniglib.c ptstiff3.c tif22pnm.c'
TARGET=tif22pnm
LIBS="$LIBS_TIFF"
build

SOURCES='png22pnm.c'
TARGET=png22pnm
LIBS="$LIBS_PNG"
build
