AUTOMAKE_OPTIONS = subdir-objects

ACLOCAL_AMFLAGS =  -I m4
AM_CFLAGS = -std=gnu11 -g -O3 -flto -fuse-linker-plugin -I$(srcdir)/src

SOURCE_FILES = \
src/picohttpparser/picohttpparser.c \
src/reactor/reactor.c \
src/reactor/reactor_assert.c \
src/reactor/reactor_vector.c \
src/reactor/reactor_utility.c \
src/reactor/reactor_user.c \
src/reactor/reactor_core.c \
src/reactor/reactor_stats.c \
src/reactor/reactor_fd.c \
src/reactor/reactor_timer.c \
src/reactor/reactor_pool.c \
src/reactor/reactor_resolver.c \
src/reactor/reactor_net.c \
src/reactor/reactor_stream.c \
src/reactor/reactor_http.c \
src/reactor/reactor_couch.c \
src/reactor/reactor_server.c

HEADER_FILES = \
src/picohttpparser/picohttpparser.h \
src/reactor/reactor.h \
src/reactor/reactor_assert.h \
src/reactor/reactor_vector.h \
src/reactor/reactor_utility.h \
src/reactor/reactor_user.h \
src/reactor/reactor_core.h \
src/reactor/reactor_stats.h \
src/reactor/reactor_fd.h \
src/reactor/reactor_timer.h \
src/reactor/reactor_pool.h \
src/reactor/reactor_resolver.h \
src/reactor/reactor_net.h \
src/reactor/reactor_stream.h \
src/reactor/reactor_http.h \
src/reactor/reactor_couch.h \
src/reactor/reactor_server.h

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
example/example \
example/producer \
example/resolver \
example/tcp \
example/tcpd \
example/http \
example/couch \
example/httpd \
example/server \
example/hello

example_example_LDFLAGS = -pthread
example_example_LDADD = libreactor.la -ldynamic

example_producer_LDFLAGS = -pthread
example_producer_LDADD = libreactor.la -ldynamic

example_resolver_LDFLAGS = -pthread
example_resolver_LDADD = libreactor.la -ldynamic

example_tcp_LDFLAGS = -pthread
example_tcp_LDADD = libreactor.la -ldynamic

example_tcpd_LDFLAGS = -pthread
example_tcpd_LDADD = libreactor.la -ldynamic

example_http_LDFLAGS = -pthread
example_http_LDADD = libreactor.la -ldynamic

example_httpd_LDFLAGS = -pthread
example_httpd_LDADD = libreactor.la -ldynamic

example_couch_LDFLAGS = -pthread
example_couch_LDADD = libreactor.la -ldynamic -ljansson

example_server_LDFLAGS = -pthread
example_server_LDADD = libreactor.la -ldynamic -ljansson

example_hello_LDFLAGS = -pthread
example_hello_LDADD = libreactor.la -ldynamic -ljansson

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in libreactor-?.?.?.tar.gz

maintainer-clean-local:
	rm -rf autotools m4 libreactor-?.?.?

CLEANFILES = {.,test/,src/reactor/,src/picohttpparser}/*.{gcno,gcda,gcov}

### unit tests ###

CHECK_CFLAGS = -std=gnu11 -O0 -g -ftest-coverage -fprofile-arcs -I$(srcdir)/src -DGCOV_BUILD
CHECK_LDADD = -L. libreactor_test.a -lcmocka -ldynamic
CHECK_LDFLAGS_EXTRA = -pthread -Wl,--wrap=abort

check_LIBRARIES = libreactor_test.a
libreactor_test_a_CFLAGS = $(CHECK_CFLAGS)
libreactor_test_a_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

check_PROGRAMS =

check_PROGRAMS += test/reactor
test_reactor_CFLAGS = $(CHECK_CFLAGS)
test_reactor_LDADD = $(CHECK_LDADD)
test_reactor_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_SOURCES = test/reactor.c test/mock.c

check_PROGRAMS += test/reactor_assert
test_reactor_assert_CFLAGS = $(CHECK_CFLAGS)
test_reactor_assert_LDADD = $(CHECK_LDADD)
test_reactor_assert_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_assert_SOURCES = test/reactor_assert.c test/mock.c

check_PROGRAMS += test/reactor_core
test_reactor_core_CFLAGS = $(CHECK_CFLAGS)
test_reactor_core_LDADD = $(CHECK_LDADD)
test_reactor_core_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_core_SOURCES = test/reactor_core.c test/mock.c

check_PROGRAMS += test/reactor_pool
test_reactor_pool_CFLAGS = $(CHECK_CFLAGS)
test_reactor_pool_LDADD = $(CHECK_LDADD)
test_reactor_pool_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_pool_SOURCES = test/reactor_pool.c test/mock.c

check_PROGRAMS += test/reactor_resolver
test_reactor_resolver_CFLAGS = $(CHECK_CFLAGS)
test_reactor_resolver_LDADD = $(CHECK_LDADD)
test_reactor_resolver_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_resolver_SOURCES = test/reactor_resolver.c test/mock.c

dist_noinst_SCRIPTS = test/valgrind.sh test/coverage.sh Dockerfile

TESTS = $(check_PROGRAMS) test/valgrind.sh test/coverage.sh

hello:
	docker build -t reactor-sdk .
	docker run --rm -v ${PWD}/example:/example reactor-sdk \
	gcc -Wall -Wpedantic -flto -O2 -pthread -static -no-pie \
	example/hello.c -o example/hello \
	-lreactor -ldynamic
	strip example/hello
