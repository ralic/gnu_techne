scriptsdir = ${pkgdatadir}/scripts

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src

if BUILD_TOOLS
   SUBDIRS += tools
endif

dist_doc_DATA = README
dist_man1_MANS = doc/techne.1

if BUILD_BROWSER
   dist_man1_MANS += doc/techne-browser.1
endif

nobase_pkgdata_DATA = scripts/utils/basic.lua \
		      scripts/utils/orbit.lua \
		      scripts/tests/atmosphere.lua \
		      scripts/tests/elevation.lua \
		      scripts/tests/vegetation.lua \
		      scripts/tests/joints.lua \
		      scripts/tests/widgets.lua \
		      scripts/tests/graph.lua \
		      scripts/tests/diamondsquare.lua \
		      scripts/tests/springs.lua \
		      scripts/tests/textured.lua \
		      scripts/tests/bezier.lua
