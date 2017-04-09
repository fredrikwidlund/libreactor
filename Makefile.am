ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -g -O3 -flto -fuse-linker-plugin -I$(srcdir)/src
AM_LDFLAGS = -static

DIST_SUBDIRS = docs

EXTRA_DIST = \
CHANGES \
LICENSE \
README.rst

SOURCE_FILES = \
src/picohttpparser/picohttpparser.c \
src/reactor/reactor_memory.c \
src/reactor/reactor_user.c \
src/reactor/reactor_pool.c \
src/reactor/reactor_core.c \
src/reactor/reactor_timer.c \
src/reactor/reactor_stream.c \
src/reactor/reactor_resolver.c \
src/reactor/reactor_tcp.c \
src/reactor/reactor_http_parser.c \
src/reactor/reactor_http.c \
src/reactor/reactor_http_server.c

HEADER_FILES = \
src/reactor/reactor_memory.h \
src/reactor/reactor_util.h \
src/reactor/reactor_user.h \
src/reactor/reactor_pool.h \
src/reactor/reactor_core.h \
src/reactor/reactor_timer.h \
src/reactor/reactor_stream.h \
src/reactor/reactor_resolver.h \
src/reactor/reactor_tcp.h \
src/reactor/reactor_http_parser.h \
src/reactor/reactor_http.h \
src/reactor/reactor_http_server.h

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

check_PROGRAMS += test/reactor_resolver
test_reactor_resolver_CFLAGS = $(CHECK_CFLAGS)
test_reactor_resolver_LDADD = $(CHECK_LDADD)
test_reactor_resolver_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_resolver_SOURCES = test/reactor_resolver.c test/mock.c

dist_noinst_SCRIPTS = test/valgrind.sh test/coverage.sh

TESTS = $(check_PROGRAMS) test/valgrind.sh test/coverage.sh

## examples ###

bin_PROGRAMS = \
example/pool \
example/lookup \
example/tcp \
example/http \
example/hello \
example/timer

example_pool_LDFLAGS = -lreactor -ldynamic
example_lookup_LDFLAGS = -lreactor -ldynamic
example_tcp_LDFLAGS = -lreactor -ldynamic
example_http_LDFLAGS = -lreactor -ldynamic
example_hello_LDFLAGS = -lreactor -ldynamic
example_timer_LDFLAGS = -lreactor -ldynamic

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in docs/Makefile.in libreactor-?.?.?.tar.gz
maintainer-clean-local:; rm -rf autotools m4 libreactor-?.?.?

CLEANFILES = {.,src/reactor,test}/*.{gcno,gcda,gcov}
