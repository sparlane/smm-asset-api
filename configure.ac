AC_INIT([libsmm-asset], [0.1.0])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_CONFIG_MACRO_DIRS([m4])

AC_PROG_CC
AM_PROG_AR

LT_INIT([disable-static pic-only])

PKG_CHECK_MODULES([TIDY],[tidy])
PKG_CHECK_MODULES([CURL],[libcurl])
PKG_CHECK_MODULES([JANSSON],[jansson])

AC_CONFIG_FILES([Makefile
	src/Makefile
	src/smm-asset.pc])
AC_OUTPUT
