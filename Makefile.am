AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -g -O3 -march=native -flto -I$(srcdir)/src
AM_LDFLAGS = -static

SOURCE_FILES = \
src/picohttpparser/picohttpparser.c \
src/reactor/reactor.c \
src/reactor/timer.c \
src/reactor/stream.c \
src/reactor/notify.c \
src/reactor/http.c \
src/reactor/server.c

HEADER_FILES = \
src/picohttpparser/picohttpparser.h \
src/reactor/reactor.h \
src/reactor/timer.h \
src/reactor/stream.h \
src/reactor/notify.h \
src/reactor/http.h \
src/reactor/server.h

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
example/timer \
example/stream \
example/notify \
example/httpd_lowlevel \
example/server \
example/benchmark_http

example_timer_LDADD = libreactor.la -ldynamic
example_stream_LDADD = libreactor.la -ldynamic
example_notify_LDADD = libreactor.la -ldynamic
example_httpd_lowlevel_LDADD = libreactor.la -ldynamic
example_server_LDADD = libreactor.la -ldynamic
example_benchmark_http_LDADD = libreactor.la -ldynamic

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in libreactor-?.?.?.tar.gz

maintainer-clean-local:
	rm -rf autotools m4 libreactor-?.?.?

CLEANFILES = {.,test/,src/reactor/,src/picohttpparser}/*.{gcno,gcda,gcov}

### unit tests ###

check_LIBRARIES = libreactor_test.a
libreactor_test_a_CFLAGS = $(CHECK_CFLAGS)
libreactor_test_a_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

CHECK_CFLAGS = -std=gnu2x -O0 -g -ftest-coverage -fprofile-arcs -I$(srcdir)/src -DGCOV_BUILD
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

check_PROGRAMS += test/stream
test_stream_CFLAGS = $(CHECK_CFLAGS)
test_stream_LDADD = $(CHECK_LDADD)
test_stream_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_stream_SOURCES = test/stream.c test/mock.c

check_PROGRAMS += test/http
test_http_CFLAGS = $(CHECK_CFLAGS) -DSRCDIR=\"$(srcdir)/\"
test_http_LDADD = $(CHECK_LDADD)
test_http_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_http_SOURCES = test/http.c test/mock.c

dist_noinst_SCRIPTS = test/valgrind.sh test/coverage.sh Dockerfile

TESTS = $(check_PROGRAMS) test/valgrind.sh test/coverage.sh

hello:
	docker build -t reactor-sdk .
	docker run --rm -v ${PWD}:/work reactor-sdk \
	gcc -Wall -Wpedantic -march=native -flto -O3 -static /work/example/server.c -o /work/hello -lreactor -ldynamic && \
	strip hello

indent:
	clang-format -i src/reactor/*.c test/*.c example/*.c
