.PHONY: all install distclean dist compile clean

compile: compiled.stamp

all compiled.stamp: config.h
	./do.sh compile
	touch compiled.stamp

config.h:
	./configure

distclean:
	./do.sh confclean

install: compiled.stamp
	test -f tif22pnm || exit 0; . cc_help.sh && cp tif22pnm "$$prefix"/bin
	test -f png22pnm || exit 0; . cc_help.sh && cp png22pnm "$$prefix"/bin

dist: distclean
	./do.sh dist

clean:
	./do.sh clean
