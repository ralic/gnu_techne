noinst_PROGRAMS = pngtoarray pgmtoelevation pnmtoimagery

pngtoarray_SOURCES = pngtoarray.c
pngtoarray_CFLAGS = ${AM_CFLAGS} ${LIBPNG_CFLAGS}
pngtoarray_LDADD = ${LIBPNG_LIBS} ${AM_LDADD} -lm

pgmtoelevation_SOURCES = pgmtoelevation.c
pgmtoelevation_CFLAGS = ${AM_CFLAGS}
pgmtoelevation_LDADD = ${LIBPNG_LIBS} ${AM_LDADD} -lm -lnetpbm

pnmtoimagery_SOURCES = pnmtoimagery.c
pnmtoimagery_CFLAGS = ${AM_CFLAGS}
pnmtoimagery_LDADD = ${LIBPNG_LIBS} ${AM_LDADD} -lm -lnetpbm
