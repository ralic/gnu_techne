PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib/lua/5.2/transform
SHAREDIR=$(PREFIX)/share/lua/5.2

LUA_CFLAGS=`pkg-config --cflags lua5.2` -I..
LUA_LDFLAGS=`pkg-config --libs lua5.2`

CFLAGS=-fpic -g -Wall -std=c99 -pedantic
LDFLAGS=-shared -fpic

INSTALL=/usr/bin/install

SOURCES=core.c ../array/array.c ../array/operations.c

core: $(SOURCES)
	$(CC) -o core.so ${CFLAGS} ${LDFLAGS} $(SOURCES) ${LUA_CFLAGS} ${LUA_LDFLAGS}

install: core
	mkdir -p $(LIBDIR)/
	$(INSTALL) core.so $(LIBDIR)/
	mkdir -p $(SHAREDIR)/
	$(INSTALL) transform.lua $(SHAREDIR)

uninstall:
	rm -f $(LIBDIR)/core.so
	rm -f $(SHAREDIR)/transform.lua

dist: core
	if [ -e /tmp/transform ]; then rm -rf /tmp/transform; fi
	mkdir /tmp/transform
	cp transform.lua core.c Makefile README /tmp/transform
	cd /tmp; tar zcf luatransform.tar.gz transform/

clean:
	rm -f core.so *~