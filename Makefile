.PHONY: all install distclean dist

all:
	[ -f config.h ] || ./configure
	./do.sh compile

distclean:
	./do.sh confclean

install:
	cp tif22pnm $(prefix)/bin
	cp png22pnm $(prefix)/bin

dist: distclean
	./do.sh dist
