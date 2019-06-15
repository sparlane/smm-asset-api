AM_CFLAGS = -Werror -Wall -Wformat=2 -Wvla -Wextra -Wwrite-strings -Wmissing-prototypes -Wunreachable-code -pedantic -std=c99 -D_DEFAULT_SOURCE -D_GNU_SOURCE

lib_LTLIBRARIES = libsmmasset.la

libsmmasset_la_SOURCES = smm-asset.c

include_HEADERS = smm-asset.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = smm-asset.pc
