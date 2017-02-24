ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -g -O3 -flto -fuse-linker-plugin -I$(srcdir)/src
AM_LDFLAGS = -static

DIST_SUBDIRS = docs

EXTRA_DIST = \
CHANGES \
LICENSE \
README.rst

SOURCE_FILES = \
src/reactor/reactor_user.c \
src/reactor/reactor_core.c \
src/reactor/reactor_stream.c

HEADER_FILES = \
src/reactor/reactor_user.h \
src/reactor/reactor_core.h \
src/reactor/reactor_stream.h

AUTOMAKE_OPTIONS = subdir-objects
lib_LTLIBRARIES = libreactor.la
libreactor_la_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

headerfilesdir = $(includedir)/reactor
headerfiles_HEADERS = $(HEADER_FILES)

mainheaderdir = $(includedir)
mainheader_HEADERS = src/reactor.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libreactor.pc

### unit tests ###

CHECK_CFLAGS = -std=gnu11 -O0 -g -ftest-coverage -fprofile-arcs -I$(srcdir)/src
CHECK_LDADD = -L. -lreactor_test -lcmocka -ldynamic
CHECK_LDFLAGS_EXTRA = -Wl,--wrap=poll -Wl,--wrap=read

check_LIBRARIES = libreactor_test.a
libreactor_test_a_CFLAGS = $(CHECK_CFLAGS)
libreactor_test_a_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

check_PROGRAMS =

check_PROGRAMS += test/reactor_user
test_reactor_user_CFLAGS = $(CHECK_CFLAGS)
test_reactor_user_LDADD = $(CHECK_LDADD)
test_reactor_user_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_user_SOURCES = test/reactor_user.c test/mock.c

check_PROGRAMS += test/reactor_core
test_reactor_core_CFLAGS = $(CHECK_CFLAGS)
test_reactor_core_LDADD = $(CHECK_LDADD)
test_reactor_core_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_core_SOURCES = test/reactor_core.c test/mock.c

check_PROGRAMS += test/reactor_stream
test_reactor_stream_CFLAGS = $(CHECK_CFLAGS)
test_reactor_stream_LDADD = $(CHECK_LDADD)
test_reactor_stream_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_stream_SOURCES = test/reactor_stream.c test/mock.c

dist_noinst_SCRIPTS = test/valgrind.sh test/coverage.sh

TESTS = $(check_PROGRAMS) test/valgrind.sh test/coverage.sh

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in docs/Makefile.in libreactor-?.?.?.tar.gz
maintainer-clean-local:; rm -rf autotools m4 libreactor-?.?.?

CLEANFILES = {.,src/reactor,test}/*.{gcno,gcda,gcov}
