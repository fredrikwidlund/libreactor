AC_INIT([libreactor], [1.0.0], [fredrik.widlund@gmail.com])
AC_CONFIG_AUX_DIR(autotools)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign no-define])

: ${CFLAGS="-Wall -Wextra -Wpedantic"}

AM_PROG_AR
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

AC_CONFIG_FILES([Makefile docs/Makefile libreactor.pc])
AC_OUTPUT
