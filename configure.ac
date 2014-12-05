#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(techne, 0.3)
AC_PREREQ(2.61)

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/main.c])

AC_GNU_SOURCE
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([nostdinc subdir-objects -Wall -Werror])

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

# Checks for programs.

AC_PROG_CC
AM_PROG_CC_C_O
AM_PROG_AR
AC_PROG_LN_S

LT_INIT([dlopen])

# Checks for libraries.

AX_LIB_READLINE
AC_CHECK_LIB(dw, dwfl_errno)

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
PKG_CHECK_MODULES([GL], [x11 gl glu])
PKG_CHECK_MODULES([MHD], [libmicrohttpd])

dnl Check whether building of the tools was requested and add needed
dnl dependencies.

AC_ARG_ENABLE([tools], AS_HELP_STRING([--enable-tools],
                                    [Build the development tools.]))
AM_CONDITIONAL([BUILD_TOOLS], [test x$enable_tools = xyes])

if test "x$enable_tools" = "xyes"; then
   PKG_CHECK_MODULES([LIBPNG], [libpng])

   AC_CONFIG_FILES([tools/Makefile])
fi

AC_ARG_ENABLE([browser], AS_HELP_STRING([--enable-browser],
                                      [Build the browser.]))
AM_CONDITIONAL([BUILD_BROWSER], [test x$enable_browser = xyes])

if test "x$enable_browser" = "xyes"; then
   PKG_CHECK_MODULES([WEBKIT], [webkit-1.0])
fi

# Checks for library functions.

AC_CHECK_FUNCS([backtrace ioctl asprintf])

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 src/techne])
AC_OUTPUT
