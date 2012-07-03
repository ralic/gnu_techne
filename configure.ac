#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(techne, 0.3)
AC_PREREQ(2.61)

AC_GNU_SOURCE

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_LIBOBJ_DIR([src/compat])

AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([nostdinc -Wall -Werror])

LT_PREREQ([2.2])

AS_CASE([$host_os],
	[linux-gnu], [	
		AC_ENABLE_SHARED
		AC_DISABLE_STATIC
	],
	[mingw32], [
		AC_DISABLE_SHARED
		AC_ENABLE_STATIC
	],
	[AC_ERROR([Unsupported host $host_os])]
)

LT_INIT([dlopen])
# Checks for programs.

AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.

AX_LIB_READLINE

# Lua tends to be named differently among distributions,
# so we need to look for the proper packagename.

PKG_PROG_PKG_CONFIG

luaname=$(
  for p in `$PKG_CONFIG --silence-errors --list-all |
              grep -i lua |  cut -d' ' -f1`; do
    if $PKG_CONFIG --atleast-version=5.2 $p; then
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
PKG_CHECK_MODULES([ODE], [ode])
PKG_CHECK_MODULES([GDK], [gdk-2.0])
PKG_CHECK_MODULES([GDKGLEXT], [gdkglext-1.0])
PKG_CHECK_MODULES([MHD], [libmicrohttpd])
PKG_CHECK_MODULES([WEBKIT], [webkit-1.0])
dnl PKG_CHECK_MODULES([MAGICKWAND], [MagickWand])
dnl PKG_CHECK_MODULES([GVC], [libgvc])

# Checks for library functions.

AC_CHECK_FUNCS([backtrace wordexp ioctl asprintf])

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 src/techne])
AC_OUTPUT
