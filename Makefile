SHELL = /bin/sh
INSTALL = /usr/bin/install -c
INSTALL_DATA = /usr/bin/install -c -m 644

prefix = /usr/local
exec_prefix = $(prefix)
libdir = $(exec_prefix)/lib
includedir = $(prefix)/include
datarootdir = $(prefix)/share
docdir = $(datarootdir)/doc/libplurc
sysconfdir = $(prefix)/etc

sslcertdir = $(sysconfdir)/ssl/cert

CFLAGS+= -Wall -Wextra -Werror -I. -I/usr/include -g
LDFLAGS+= -L$(DESTDIR)$(libdir) -Xlinker "-rpath=$(DESTDIR)$(libdir)" -L. -Xlinker "-rpath=."

all: library

library:
	${CC} ${CFLAGS} ${LDFLAGS} -shared -fPIC -lssl -lm -o libplurc.so libplurc.c json.c -DSSL_CERT_PATH=$(sslcertdir)
	${CC} ${CFLAGS} ${LDFLAGS} -fPIC -shared -o libplurc.dylib libplurc.so

install: library
	mkdir -p $(DESTDIR)$(libdir)
	$(INSTALL_DATA) libplurc.so $(DESTDIR)$(libdir)/libplurc.so
	$(INSTALL_DATA) libplurc.dylib $(DESTDIR)$(libdir)/libplurc.dylib
	mkdir -p $(DESTDIR)$(includedir)
	$(INSTALL_DATA) libplurc.h $(DESTDIR)$(includedir)/libplurc.h
	$(INSTALL_DATA) json.h $(DESTDIR)$(includedir)/json.h
	mkdir -p $(DESTDIR)$(sslcertdir)
	$(INSTALL_DATA)  Equifax_Secure_Plurc_CA.crt $(DESTDIR)$(sslcertdir)/Equifax_Secure_Plurc_CA.crt
	mkdir -p $(DESTDIR)$(docdir)
	$(INSTALL_DATA)  README $(DESTDIR)$(docdir)/README

plurc: install plurc.o config.h
	$(CC) $(LDFLAGS) -lplurc -o plurc plurc.o

dumplurk: install dumplurk.o config.h
	$(CC) $(LDFLAGS) -lplurc -o dumplurk dumplurk.o

clean:
	rm -f libplurc.dylib libplurc.so plurc *.o

uninstall:
	rm -rf $(DESTDIR)$(libdir)/libplurc.so \
	       $(DESTDIR)$(libdir)/libplurc.dylib \
	       $(DESTDIR)$(includedir)/libplurc.h \
	       $(DESTDIR)$(includedir)/json.h \
	       $(DESTDIR)$(sslcertdir)/Equifax_Secure_Plurc_CA.crt \
	       $(DESTDIR)$(docdir)/README

.PHONY: library install plurc clean uninstall

