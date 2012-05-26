#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(techne, 0.3)
AC_PREREQ(2.61)

AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([nostdinc -Wall -Werror])

AC_CONFIG_SRCDIR([src/main.c])
AC_GNU_SOURCE

# Checks for programs.

AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.

AX_LIB_READLINE

# Lua tends to be named differently among distributions,
# so we need to look for the proper packagename.

luaname=$(
  for p in `pkg-config --silence-errors --list-all |
              grep -i lua |  cut -d' ' -f1`; do
    if pkg-config --atleast-version=5.2 $p; then
      echo $p
      break
    fi
  done

  if test -z $luaname; then
    luaname=lua
  fi
)

PKG_CHECK_MODULES([LUA], [${luaname}])

if test -z "${LUAC}"; then
   AC_CHECK_PROGS(LUAC, [luac5.2 luac52 luac])

   if test -z "`${LUAC} -v | grep "Lua 5.2"`"; then  
      AC_MSG_ERROR([
Lua 5.2 was found but the default Lua compiler does not seem to
be version 5.2.  You have to manually set the variable LUAC to the
path to the Lua 5.2 compiler.
      ])
   fi
fi

PKG_CHECK_MODULES([OPENAL], [openal])
PKG_CHECK_MODULES([MAGICKWAND], [MagickWand])
dnl PKG_CHECK_MODULES([PLPLOT], [plplotd])
PKG_CHECK_MODULES([GDK], [gdk-2.0])
PKG_CHECK_MODULES([GDKGLEXT], [gdkglext-1.0])
dnl PKG_CHECK_MODULES([WEBKIT], [webkit-1.0])
PKG_CHECK_MODULES([MHD], [libmicrohttpd])
dnl PKG_CHECK_MODULES([GVC], [libgvc])

# Check if the API for plarc has changed.

AC_MSG_CHECKING([whether the plarc hack is needed])
old_LDFLAGS="$LDFLAGS"
old_CFLAGS="$CFLAGS"
LDFLAGS="$LDFLAGS ${PLPLOT_LDFLAGS} ${PLPLOT_LIBS}"
CFLAGS="$CFLAGS ${PLPLOT_CFLAGS}"
AC_LANG_ASSERT([C])
AC_LINK_IFELSE(
       [AC_LANG_PROGRAM([[#include <plplot.h>]],
                        [[plarc(0, 0, 0, 0, 0, 0, 0);]])],
       [plplot_new_plarc=no],
       [plplot_new_plarc=yes])
AC_MSG_RESULT([$plplot_new_plarc])
LDFLAGS="$old_LDFLAGS"
CFLAGS="$old_CFLAGS"
unset old_LDFLAGS
unset old_CFLAGS

if test "${plplot_new_plarc}" = "yes"; then
  AC_DEFINE_UNQUOTED([PLPLOT_NEW_PLARC], [1])
fi

# Check if ODE is installed.

if test -z "${ODE_CFLAGS}" || test -z "${ODE_LIBS}"; then
   AC_CHECK_PROGS(ODE_CONFIG, [ode-double-config ode-config])

   if test -n "${ODE_CONFIG}"; then
      ODE_CFLAGS=`${ODE_CONFIG} --cflags`
      ODE_LIBS=`${ODE_CONFIG} --libs`
   else
      ODE_CFLAGS="-I/usr/include"
      ODE_LIBS="-L/usr/lib -lode"

      AC_MSG_WARN([

ODE configuration utility `ode-config' not found.  Consider setting
the variables ODE_CFLAGS and ODE_LIBS manually if the default values
don't work.
      ])
   fi

   AC_SUBST(ODE_CFLAGS)
   AC_SUBST(ODE_LIBS)
fi

# Checks for header files.

AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST

# Checks for library functions.

AC_CHECK_FUNCS([clock_gettime strdup])

AC_CONFIG_FILES([Makefile src/Makefile src/techne])
AC_OUTPUT
