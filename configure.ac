AC_INIT([Reactor pattern library], [0.1.0], [fredrik.widlund@gmail.com], [libreactor], [https://github.com/fredrikwidlund/libreactor])
AC_CONFIG_AUX_DIR(autotools)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.12 -Wall -Werror no-define])

CFLAGS="$CFLAGS -Wall -Wextra -Wpedantic"

AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AM_PROG_AR
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Check if -ffat-lto-objects should be used
saved_flags="$CFLAGS"
use_fat_lto_objects_flags=no
CFLAGS="-flto -ffat-lto-objects"
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM([[#include <stdio.h>]],[[printf("hello world\n");]])],
	[use_fat_lto_objects_flags=yes])
if test "x$use_fat_lto_objects_flags" != "xno" ;then
   AC_DEFINE([HAVE_FAT_LTO_OBJECTS], [1], [Define to 1 if the compiler LTO flag -ffat-lto-objects works.])
fi
AM_CONDITIONAL(HAVE_FAT_LTO_OBJECTS, test "x$use_fat_lto_objects_flags" != "xno")
CFLAGS="$saved_flags"

# Check if __builtin_expect is supported
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM(
		[[int foo (int a) { a = __builtin_expect (a, 10); return a == 10 ? 0 : 1; }]],
		[[]])],
    		[AC_DEFINE([HAVE_BUILTIN_EXPECT], [1], [Define to 1 if the compiler understands __builtin_expect.])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
