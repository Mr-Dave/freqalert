VERSION=0.1.0

prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib

INSTALL_PREFIX=/usr/local
CC=gcc
includedir=${prefix}/include

CFLAGS+=-g -O2 -Wall
LIBDIR = -I${includedir}
LIBDIR+= -L${libdir}  -lsndfile -lportaudio -lfftw3

OBJSli= freqalert.o pa_devs.o parms.o

all: freqalert

freqalert: $(OBJSli)
	$(CC) -W $(OBJSli) -o freqalert $(LIBDIR) -lm -DCONFIGFILE=$(INSTALL_PREFIX)/etc/freqalert.conf

install: freqalert
	cp freqalert $(INSTALL_PREFIX)/bin
	cp freqalert.conf $(INSTALL_PREFIX)/etc/

uninstall: clean
	rm -f $(INSTALL_PREFIX)/bin/freqalert
	rm -f $(INSTALL_PREFIX)/etc/freqalert.conf

clean:
	rm -f $(OBJSsl) $(OBJSli) $(OBJSsp) freqalert core

package: clean
	# source package
	rm -rf freqalert-$(VERSION)
	mkdir freqalert-$(VERSION)
	cp *.c *.h *.conf Makefile *.html license.txt freqalert-$(VERSION)
	tar vczf freqalert-$(VERSION).tgz freqalert-$(VERSION)
	rm -rf freqalert-$(VERSION)
