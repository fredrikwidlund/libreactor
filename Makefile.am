AUTOMAKE_OPTIONS = subdir-objects

ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -g -O3 -flto -I$(srcdir)/src
AM_LDFLAGS = -static -pthread

SOURCE_FILES = \
src/notify/notify.c

HEADER_FILES = \
src/reactor.h \
src/notify/notify.h

lib_LTLIBRARIES = libreactor.la
libreactor_la_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)
libreactor_la_LDFLAGS = -static -ldynamic

headerfilesdir = $(includedir)/reactor
headerfiles_HEADERS = $(HEADER_FILES)

mainheaderdir = $(includedir)
mainheader_HEADERS = src/reactor.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libreactor.pc

bin_PROGRAMS = \
example/notify

example_notify_LDFLAGS = -pthread
example_notify_LDADD = libreactor.la -ldynamic

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in libreactor-?.?.?.tar.gz

maintainer-clean-local:
	rm -rf autotools m4 libreactor-?.?.?

CLEANFILES = {.,test/,src/reactor/,src/picohttpparser}/*.{gcno,gcda,gcov}

### unit tests ###

check_LIBRARIES = libreactor_test.a
libreactor_test_a_CFLAGS = $(CHECK_CFLAGS)
libreactor_test_a_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

CHECK_CFLAGS = -std=gnu11 -O0 -g -ftest-coverage -fprofile-arcs -I$(srcdir)/src -DGCOV_BUILD
CHECK_LDADD = -L. libreactor_test.a -lcmocka -ldynamic
CHECK_LDFLAGS_EXTRA = -pthread \
-Wl,--wrap=abort \
-Wl,--wrap=read \
-Wl,--wrap=inotify_init1

check_PROGRAMS =

check_PROGRAMS += test/notify
test_notify_CFLAGS = $(CHECK_CFLAGS)
test_notify_LDADD = $(CHECK_LDADD)
test_notify_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_notify_SOURCES = test/notify.c test/mock.c

dist_noinst_SCRIPTS = test/valgrind.sh test/coverage.sh Dockerfile

TESTS = $(check_PROGRAMS) test/valgrind.sh test/coverage.sh

hello:
	docker build -t reactor-sdk .
	docker run --rm -v ${PWD}/example:/example reactor-sdk \
	gcc -Wall -Wpedantic -flto -O2 -pthread -static -no-pie \
	example/hello.c -o example/hello \
	-lreactor -ldynamic
	strip example/hello
