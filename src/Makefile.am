mybindir = $(pkglibdir)
modulesdir = ${pkgdatadir}/modules

AM_CPPFLAGS = -x objective-c \
	      -D_GNU_SOURCE -DGL_GLEXT_PROTOTYPES \
	      -DPKGDATADIR='"$(pkgdatadir)"' -DPKGLIBDIR='"$(pkglibdir)"'
AM_CFLAGS = -Wall -Wno-div-by-zero -I$(srcdir) ${ODE_CFLAGS}
AM_LDFLAGS = -lobjc -lm -lrt -ldl

bin_SCRIPTS = techne

nobase_mybin_PROGRAMS = techne.bin joints/core.so	\
		        array/core.so transform/core.so			\
		        bodies/core.so primitives/core.so physics.so	\
		        shapes/core.so automotive/core.so		\
		        widgets/core.so
# console/core.so

dist_modules_DATA = joints/joints.lua		\
		    array/array.lua transform/transform.lua		\
		    bodies/bodies.lua primitives/primitives.lua		\
		    shapes/shapes.lua misc/bindings.lua			\
		    misc/units.lua misc/resources.lua			\
		    automotive/automotive.lua widgets/widgets.lua
# console/console.lua

techne_bin_SOURCES = techne.c techne.h main.c object.c object.h		\
		     node.c node.h graphics.c graphics.h accoustics.c	\
		     accoustics.h dynamics.c dynamics.h transform.c	\
		     transform.h body.c body.h joint.c joint.h		\
		     event.c event.h space.c space.h array/array.c	\
		     array/array.h prompt/prompt.c prompt/prompt.h	\
		     joints/contact.c profiling.c moremath.c

techne_bin_CFLAGS = ${LUA_CFLAGS} ${AM_CFLAGS} ${GDKGLEXT_CFLAGS}	\
		    ${OPENAL_CFLAGS} ${ODE_CFLAGS}
techne_bin_LDFLAGS = -rdynamic ${LUA_LIBS} ${GDKGLEXT_LIBS}		\
		     ${OPENAL_LIBS} ${AM_LDFLAGS} ${ODE_LIBS}

# console_core_so_SOURCES = console/core.c
# console_core_so_CFLAGS = -fpic ${AM_CFLAGS} ${GVC_CFLAGS} ${LUA_CFLAGS}
# console_core_so_LDFLAGS = -shared -fpic ${AM_LDFLAGS} ${GVC_LIBS}	\
# 			  ${LUA_LIBS}

array_core_so_SOURCES = array/core.c array/array.h
array_core_so_CFLAGS = -fpic -Wall ${LUA_CFLAGS} ${AM_CFLAGS}
array_core_so_LDFLAGS = -shared -fpic ${LUA_LIBS}

transform_core_so_SOURCES = transform/core.c
transform_core_so_CFLAGS = -fpic -Wall ${LUA_CFLAGS} ${AM_CFLAGS}
transform_core_so_LDFLAGS = -shared -fpic -lm ${LUA_LIBS}

joints_core_so_SOURCES = joints/core.c joints/clamp.c joints/clamp.h	\
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
joints_core_so_CFLAGS = -fpic ${AM_CFLAGS} ${LUA_CFLAGS}		\
			 ${ODE_CFLAGS}
joints_core_so_LDFLAGS = -shared -fpic ${AM_LDFLAGS} ${LUA_LIBS}	\
			 ${ODE_LIBS}

bodies_core_so_SOURCES = bodies/core.c bodies/point.c bodies/point.h	\
			 bodies/box.c bodies/box.h			\
			 bodies/environment.c bodies/environment.h	\
			 bodies/plane.c bodies/plane.h			\
			 bodies/capsule.h bodies/capsule.c		\
			 bodies/cylinder.h bodies/cylinder.c		\
			 bodies/polyhedron.c bodies/polyhedron.h	\
			 bodies/system.c bodies/system.h		\
			 bodies/ball.c bodies/ball.h
bodies_core_so_CFLAGS = -fpic ${AM_CFLAGS} ${LUA_CFLAGS}		\
			${ODE_CFLAGS}
bodies_core_so_LDFLAGS = -shared -fpic ${AM_LDFLAGS} ${LUA_LIBS}	\
			${ODE_LIBS}

# Only the cflags for Gdk are needed here!

primitives_core_so_SOURCES = primitives/core.c primitives/observer.c	\
			     primitives/observer.h			\
			     primitives/cursor.c primitives/cursor.h
primitives_core_so_CFLAGS = -fpic ${AM_CFLAGS} ${LUA_CFLAGS}		\
			     ${GDK_CFLAGS}
primitives_core_so_LDFLAGS = -shared -fpic ${AM_LDFLAGS} ${LUA_LIBS}	\
			     ${ODE_LIBS}

physics_so_SOURCES = physics.c
physics_so_CFLAGS = -fpic ${AM_CFLAGS} ${LUA_CFLAGS} ${ODE_CFLAGS}
physics_so_LDFLAGS = -shared -fpic ${AM_LDFLAGS} ${LUA_LIBS}	\
		     ${ODE_LIBS}

shapes_core_so_SOURCES = shapes/core.c shapes/shape.c shapes/shape.h	\
			 shapes/line.c shapes/line.h shapes/polygon.c	\
			 shapes/polygon.h shapes/points.c		\
			 shapes/points.h shapes/lines.c			\
			 shapes/lines.h
shapes_core_so_CFLAGS = -fpic ${AM_CFLAGS} ${LUA_CFLAGS}		\
			 ${GDKGLEXT_CFLAGS}
shapes_core_so_LDFLAGS = -shared -fpic ${AM_LDFLAGS} ${LUA_LIBS}	\
			 ${GDKGLEXT_LIBS}

widgets_core_so_SOURCES = widgets/core.c widgets/display.c	\
			  widgets/display.h widgets/widget.c	\
			  widgets/widget.h widgets/layout.c	\
			  widgets/layout.h widgets/row.h	\
			  widgets/row.c
widgets_core_so_CFLAGS = -fpic ${AM_CFLAGS} ${LUA_CFLAGS}		\
			  ${GDKGLEXT_CFLAGS}
widgets_core_so_LDFLAGS = -shared -fpic ${AM_LDFLAGS} ${LUA_LIBS}	\
			  ${GDKGLEXT_LIBS}

automotive_core_so_SOURCES = automotive/core.c automotive/chain.c	\
		  	     automotive/chain.h				\
		  	     automotive/fourstroke.c			\
		  	     automotive/fourstroke.h			\
		  	     automotive/wheel.c automotive/wheel.h	\
		  	     automotive/racetrack.c			\
		  	     automotive/racetrack.h			\
		  	     automotive/ground.c automotive/ground.h
automotive_core_so_CFLAGS = -fpic ${AM_CFLAGS} ${LUA_CFLAGS}		\
			${ODE_CFLAGS}
automotive_core_so_LDFLAGS = -shared -fpic ${AM_LDFLAGS} ${LUA_LIBS}	\
			${ODE_LIBS}

CLEANFILES = $(bin_SCRIPTS)
