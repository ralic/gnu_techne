mybindir = $(pkglibdir)
modulesdir = ${pkgdatadir}/modules

AM_CPPFLAGS = -x objective-c \
	      -D_GNU_SOURCE -DGL_GLEXT_PROTOTYPES -DARRAYMATH_COLUMN_MAJOR\
	      -DPKGDATADIR='"$(pkgdatadir)"' -DPKGLIBDIR='"$(pkglibdir)"' \
	      -include common.h
AM_CFLAGS = -Wall -Wno-shadow -Wno-div-by-zero -I$(top_srcdir)	\
	    -I$(srcdir) ${LUA_CFLAGS}
AM_LDADD = -lobjc -lm

bin_SCRIPTS = techne

# Support programs

BUILT_SOURCES = glsl/flat_vertex.h glsl/flat_fragment.h			\
		glsl/layout_vertex.h glsl/textured_fragment.h		\
		glsl/splat_vertex.h glsl/splat_fragment.h		\
		glsl/color.h glsl/rand.h glsl/vegetation_geometry_top.h	\
		glsl/vegetation_geometry_bottom.h			\
		glsl/vegetation_vertex.h glsl/vegetation_fragment.h	\
		glsl/grass_geometry.h glsl/transform_block.h 		\
		glsl/preamble.h gl.h glx.h
SUFFIXES = .glsl.c
$(BUILT_SOURCES): glsl/stringify
.glsl.c.h: glsl/stringify
	glsl/stringify $< > $@

noinst_PROGRAMS = glsl/stringify

glsl_stringify_SOURCES = glsl/stringify.c
glsl_stringify_CPPFLAGS = 
glsl_stringify_CFLAGS = -Wall
glsl_stringify_LDFLAGS = 

opengl.c gl.h glx.h: mkopengl.sh
	./mkopengl.sh

EXTRA_DIST = $(BUILT_SOURCES:.h=.glsl.c) mkopengl.sh

# The main programs

nobase_mybin_PROGRAMS = techne.bin browser.bin

techne_bin_SOURCES = techne.c techne.h main.c object.c object.h		\
		     node.c node.h opengl.c gl.h glx.h graphics.c	\
		     graphics.h accoustics.c accoustics.h dynamics.c	\
		     dynamics.h transform.c transform.h body.c		\
		     body.h joint.c joint.h event.c event.h space.c	\
		     space.h shape.c shape.h shader.c shader.h		\
		     array/array.c arraymath/operations.c		\
		     array/array.h prompt/prompt.c prompt/prompt.h	\
		     joints/contact.c moremath.c morebase.c timing.c	\
		     memory.c network.c network.h proxy.c proxy.h	\
		     input.c input.h structures.h math.h root.c root.h	\
		     builtin.c builtin.h dynamic.c dynamic.h graphic.c  \
		     graphic.h texture.c texture.h

EXTRA_techne_bin_SOURCES = compat/asprintf.h

techne_bin_CFLAGS = ${AM_CFLAGS} ${GDK_CFLAGS} ${GL_CFLAGS}		\
		    ${OPENAL_CFLAGS} ${ODE_CFLAGS} ${MHD_CFLAGS}
techne_bin_LDFLAGS = -export-dynamic
techne_bin_LDADD = ${LUA_LIBS} ${GDK_LIBS} ${GL_LIBS} ${OPENAL_LIBS}	\
		   ${ODE_LIBS} ${MHD_LIBS} ${AM_LDADD}

browser_bin_CFLAGS = ${AM_CFLAGS} ${WEBKIT_CFLAGS} ${GTK_CFLAGS}
browser_bin_LDADD = ${WEBKIT_LIBS} ${GTK_LIBS} ${AM_LDADD}

# The modules

nobase_pkglib_LTLIBRARIES = array/core.la arraymath/core.la		\
			    joints/core.la bodies/core.la		\
			    primitives/core.la controllers/core.la	\
			    shapes/core.la widgets/core.la		\
			    automotive/core.la shading/core.la 		\
			    topography/core.la textures/core.la

pkglib_LTLIBRARIES = misc/physics.la

dist_modules_DATA = joints/joints.lua array/array.lua			\
		    arraymath/arraymath.lua bodies/bodies.lua		\
		    primitives/primitives.lua shapes/shapes.lua		\
		    misc/bindings.lua misc/units.lua			\
		    misc/resources.lua automotive/automotive.lua	\
		    widgets/widgets.lua misc/serialize.lua		\
		    shading/shading.lua controllers/controllers.lua 	\
		    topography/topography.lua textures/textures.lua	\
		    staging/staging.lua

topography_core_la_SOURCES = topography/core.c topography/elevation.c 	\
			     topography/elevation_body.c 		\
			     topography/elevation_shape.c 		\
			     topography/elevation_seeds.c		\
			     topography/elevation.h 			\
			     topography/roam.h topography/roam.c	\
			     topography/roam_vegetation.c		\
			     topography/bounds.c			\
			     topography/atmosphere.h 			\
			     topography/atmosphere.c			\
			     topography/splat.h topography/splat.c	\
			     topography/swatch.h topography/swatch.c	\
			     topography/grass.h topography/grass.c	\
			     topography/barren.h topography/barren.c	\
			     topography/vegetation.h topography/vegetation.c
topography_core_la_CFLAGS = ${AM_CFLAGS}
topography_core_la_LDFLAGS = -module -avoid-version -no-undefined
topography_core_la_LIBADD = ${LUA_LIBS} ${AM_LDADD}

controllers_core_la_SOURCES = controllers/core.c controllers/controller.c \
			      controllers/controller.h 			  \
			      controllers/device.c controllers/device.h	  \
			      controllers/keyboard.c controllers/keyboard.h \
			      controllers/pointer.c controllers/pointer.h

controllers_core_la_CFLAGS = ${AM_CFLAGS} ${GDK_CFLAGS}
controllers_core_la_LDFLAGS = -module -avoid-version -no-undefined
controllers_core_la_LIBADD = ${LUA_LIBS} ${AM_LDADD} ${GDK_LIBS}

array_core_la_SOURCES = array/core.c array/array.h
array_core_la_CFLAGS = ${AM_CFLAGS}
array_core_la_LDFLAGS = -module -avoid-version -no-undefined
array_core_la_LIBADD = ${LUA_LIBS} ${AM_LDADD}

arraymath_core_la_SOURCES = arraymath/core.c array/arraymath.h
arraymath_core_la_CFLAGS = ${AM_CFLAGS}
arraymath_core_la_LDFLAGS = -module -avoid-version -no-undefined
arraymath_core_la_LIBADD = ${LUA_LIBS} ${AM_LDADD}

bodies_core_la_SOURCES = bodies/core.c bodies/point.c bodies/point.h	\
			 bodies/box.c bodies/box.h			\
			 bodies/environment.c bodies/environment.h	\
			 bodies/plane.c bodies/plane.h			\
			 bodies/capsule.h bodies/capsule.c		\
			 bodies/cylinder.h bodies/cylinder.c		\
			 bodies/polyhedron.c bodies/polyhedron.h	\
			 bodies/system.c bodies/system.h		\
			 bodies/ball.c bodies/ball.h
bodies_core_la_CFLAGS = ${AM_CFLAGS} ${ODE_CFLAGS}
bodies_core_la_LDFLAGS = -module -avoid-version -no-undefined
bodies_core_la_LIBADD = ${ODE_LIBS} ${AM_LDADD}

joints_core_la_SOURCES = joints/core.c joints/clamp.c joints/clamp.h	\
			 joints/hinge.c joints/hinge.h			\
			 joints/doublehinge.c joints/doublehinge.h	\
			 joints/doubleball.c joints/doubleball.h	\
			 joints/spherical.c joints/spherical.h		\
			 joints/slider.c joints/slider.h		\
			 joints/planar.c joints/planar.h		\
			 joints/linear.c joints/linear.h		\
			 joints/angular.c joints/angular.h		\
			 joints/euler.c joints/euler.h			\
			 joints/universal.c joints/universal.h		\
			 joints/gearing.c joints/gearing.h		\
			 joints/contact.c joints/contact.h
joints_core_la_CFLAGS = ${AM_CFLAGS} ${ODE_CFLAGS}
joints_core_la_LDFLAGS = -module -avoid-version -no-undefined
joints_core_la_LIBADD = ${ODE_LIBS} ${AM_LDADD}

primitives_core_la_SOURCES = primitives/core.c 				\
			     primitives/observer.c primitives/observer.h\
			     primitives/timer.c primitives/timer.h
primitives_core_la_CFLAGS = ${AM_CFLAGS} ${GDK_CFLAGS}
primitives_core_la_LDFLAGS = -module -avoid-version -no-undefined
primitives_core_la_LIBADD = ${GDK_LIBS} ${AM_LDADD}

shading_core_la_SOURCES = shading/core.c shading/program.c		\
			  shading/overlay.h shading/overlay.c		\
			  shading/wireframe.h shading/wireframe.c	\
			  shading/program.h shading/flat.c		\
			  shading/flat.h shading/textured.c		\
			  shading/textured.h
shading_core_la_CFLAGS = ${AM_CFLAGS} ${GL_CFLAGS}
shading_core_la_LDFLAGS = -module -avoid-version -no-undefined
shading_core_la_LIBADD = ${GL_LIBS} ${AM_LDADD}

EXTRA_shading_core_la_SOURCES = glsl/flat_vertex.h glsl/flat_fragment.h

misc_physics_la_SOURCES = misc/physics.c
misc_physics_la_CFLAGS = ${AM_CFLAGS} ${ODE_CFLAGS}
misc_physics_la_LDFLAGS = -module -avoid-version -no-undefined
misc_physics_la_LIBADD = ${ODE_LIBS} ${AM_LDADD}

shapes_core_la_SOURCES = shapes/core.c
shapes_core_la_CFLAGS = ${AM_CFLAGS} ${GL_CFLAGS}
shapes_core_la_LDFLAGS = -module -avoid-version -no-undefined
shapes_core_la_LIBADD = ${GL_LIBS} ${AM_LDADD}

textures_core_la_SOURCES = textures/core.c
textures_core_la_CFLAGS = ${AM_CFLAGS} ${GL_CFLAGS}
textures_core_la_LDFLAGS = -module -avoid-version -no-undefined
textures_core_la_LIBADD = ${GL_LIBS} ${AM_LDADD}

widgets_core_la_SOURCES = widgets/core.c widgets/display.c	\
			  widgets/display.h widgets/widget.c	\
			  widgets/widget.h widgets/layout.c	\
			  widgets/layout.h widgets/row.h	\
			  widgets/row.c widgets/column.h	\
			  widgets/column.c widgets/assembly.h	\
			  widgets/assembly.c
widgets_core_la_CFLAGS = ${AM_CFLAGS} ${GDK_CFLAGS} ${GL_CFLAGS}
widgets_core_la_LDFLAGS = -module -avoid-version -no-undefined
widgets_core_la_LIBADD = ${GDK_LIBS} ${GL_LIBS} ${AM_LDADD}

automotive_core_la_SOURCES = automotive/core.c automotive/chain.c	\
		  	     automotive/chain.h				\
		  	     automotive/fourstroke.c			\
		  	     automotive/fourstroke.h			\
		  	     automotive/wheel.c automotive/wheel.h	\
		  	     automotive/racetrack.c			\
		  	     automotive/racetrack.h			\
			     topography/elevation_body.c 		\
			     topography/elevation.h			\
			     topography/roam.c topography/roam.h

automotive_core_la_CFLAGS = ${AM_CFLAGS} ${ODE_CFLAGS}
automotive_core_la_LDFLAGS = -module -avoid-version -no-undefined
automotive_core_la_LIBADD = ${ODE_LIBS} ${AM_LDADD}

CLEANFILES = $(bin_SCRIPTS) $(BUILT_SOURCES) opengl.c gl.h glx.h
