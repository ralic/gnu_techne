PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib/lua/5.2/array
SHAREDIR=$(PREFIX)/share/lua/5.2

LUA_CFLAGS=-I/usr/include/lua5.2
LUA_LDFLAGS=-llua5.2

CFLAGS=-fpic -g -Wall -std=c99 -pedantic
LDFLAGS=-shared -fpic

INSTALL=/usr/bin/install

core:
	$(CC) -o core.so ${CFLAGS} ${LDFLAGS} core.c array.c ${LUA_CFLAGS} ${LUA_LDFLAGS}

install: core
	mkdir -p $(LIBDIR)/
	$(INSTALL) core.so $(LIBDIR)/
	mkdir -p $(SHAREDIR)/
	$(INSTALL) array.lua $(SHAREDIR)

uninstall:
	rm -f $(LIBDIR)/core.so
	rm -f $(SHAREDIR)/array.lua

dist: core
	if [ -e /tmp/array ]; then rm -rf /tmp/array; fi
	mkdir /tmp/array
	cp array.c array.h array.lua core.c Makefile README /tmp/array
	cd /tmp; tar zcf luaarray.tar.gz array/

clean:
	rm -f core.so *~