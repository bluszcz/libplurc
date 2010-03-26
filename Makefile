SHELL = /bin/sh
INSTALL = /usr/bin/install -c
INSTALL_DATA = /usr/bin/install -c -m 644
LDCONFIG = /sbin/ldconfig

prefix = /usr/local
exec_prefix = $(prefix)
libdir = $(exec_prefix)/lib
includedir = $(prefix)/include
datarootdir = $(prefix)/share
docdir = $(datarootdir)/doc/libplurc
sysconfdir = $(prefix)/etc

sslcertdir = $(sysconfdir)/ssl/cert

CFLAGS+= -Wall -Wextra -Werror -I.

all: library

library:
	${CC} ${CFLAGS} ${LDFLAGS} -shared -fPIC -lssl -g  -o libplurc.so libplurc.c -DSSL_CERT_PATH=$(sslcertdir)
	${CC} ${CFLAGS} ${LDFLAGS} -fPIC -shared  -o libplurc.dylib -L. libplurc.so

install:
	$(INSTALL_DATA) libplurc.so $(DESTDIR)$(libdir)/libplurc.so
	$(INSTALL_DATA) libplurc.dylib $(DESTDIR)$(libdir)/libplurc.dylib
	$(INSTALL_DATA) libplurc.h $(DESTDIR)$(includedir)/libplurc.h
	$(INSTALL_DATA) api.h $(DESTDIR)$(includedir)/api.h
	mkdir -p $(DESTDIR)$(sslcertdir)
	$(INSTALL_DATA)  Equifax_Secure_Plurc_CA.crt $(DESTDIR)$(sslcertdir)/Equifax_Secure_Plurc_CA.crt
	mkdir -p $(DESTDIR)$(docdir)
	$(INSTALL_DATA)  README $(DESTDIR)$(docdir)/README
	$(LDCONFIG)

plurc: plurc.o config.h
	${CC} ${CFLAGS} ${LDFLAGS} -L. -lplurc  plurc.c  -o plurc 

.PHONY: clean
clean:	
	rm -f libplurc.dylib libplurc.so plurc plurc.o  test_json test_json.o

.PHONY: uninstall
uninstall:
	rm -rf $(DESTDIR)$(libdir)/libplurc.so $(DESTDIR)$(libdir)/libplurc.dylib $(DESTDIR)$(includedir)/libplurc.h $(DESTDIR)$(includedir)/api.h $(DESTDIR)$(sslcertdir)/Equifax_Secure_Plurc_CA.crt $(DESTDIR)$(docdir)/README
	$(LDCONFIG)
