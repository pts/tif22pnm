.PHONY: all install distclean

all:
	[ -f config.h ] || ./configure
	./do.sh compile

distclean:
	./do.sh confclean

install:
	cp tif22pnm $(prefix)/bin
	cp png22pnm $(prefix)/bin
