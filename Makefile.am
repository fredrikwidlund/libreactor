ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -O3 -g -flto -fuse-linker-plugin -I$(srcdir)/src
AM_LDFLAGS = -static

EXTRA_DIST = \
src/picohttpparser/picohttpparser.h \
test/valgrind.sh \
test/coverage.sh \
test/data/http/request-1-allow \
test/data/http/request-1-deny \
test/data/http/request-2-allow \
test/data/http/request-2-deny \
test/data/http/request-3-allow \
test/data/http/request-3-deny \
test/data/http/request-4-allow \
test/data/http/request-4-deny \
test/data/http/request-5-allow \
test/data/http/request-5-deny \
test/data/http/request-6-deny \
test/data/http/request-7-deny \
test/data/http/request-8-deny \
test/data/http/request-9-deny \
test/data/http/response-1-allow \
test/data/http/response-1-deny \
test/data/http/response-2-allow \
test/data/http/response-2-deny \
test/data/http/response-3-allow \
test/data/http/response-3-deny \
test/data/http/response-4-allow \
test/data/http/response-4-deny \
test/data/http/response-5-deny \
CHANGES \
LICENSE \
README.rst

SOURCE_FILES = \
src/picohttpparser/picohttpparser.c \
src/reactor/reactor_util.c \
src/reactor/reactor_user.c \
src/reactor/reactor_core.c \
src/reactor/reactor_descriptor.c \
src/reactor/reactor_pool.c \
src/reactor/reactor_timer.c \
src/reactor/reactor_stream.c \
src/reactor/reactor_resolver.c \
src/reactor/reactor_tcp.c \
src/reactor/reactor_http.c

HEADER_FILES = \
src/reactor/reactor_util.h \
src/reactor/reactor_user.h \
src/reactor/reactor_core.h \
src/reactor/reactor_descriptor.h \
src/reactor/reactor_pool.h \
src/reactor/reactor_timer.h \
src/reactor/reactor_stream.h \
src/reactor/reactor_resolver.h \
src/reactor/reactor_tcp.h \
src/reactor/reactor_http.h

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

CHECK_CFLAGS = -std=gnu11 -O0 -g -ftest-coverage -fprofile-arcs -I$(srcdir)/src -DGCOV_BUILD
CHECK_LDADD = -L. -lreactor_test -lcmocka -ldynamic
CHECK_LDFLAGS_EXTRA = \
-Wl,--wrap=epoll_wait \
-Wl,--wrap=epoll_pwait \
-Wl,--wrap=epoll_create1 \
-Wl,--wrap=epoll_ctl \
-Wl,--wrap=socketpair \
-Wl,--wrap=malloc \
-Wl,--wrap=abort \
-Wl,--wrap=read \
-Wl,--wrap=write \
-Wl,--wrap=accept \
-Wl,--wrap=bind \
-Wl,--wrap=listen \
-Wl,--wrap=connect \
-Wl,--wrap=socket \
-Wl,--wrap=timerfd_create \
-Wl,--wrap=timerfd_settime

check_LIBRARIES = libreactor_test.a
libreactor_test_a_CFLAGS = $(CHECK_CFLAGS)
libreactor_test_a_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

check_PROGRAMS =

check_PROGRAMS += test/reactor_util
test_reactor_util_CFLAGS = $(CHECK_CFLAGS)
test_reactor_util_LDADD = $(CHECK_LDADD)
test_reactor_util_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_util_SOURCES = test/reactor_util.c test/mock.c

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

check_PROGRAMS += test/reactor_descriptor
test_reactor_descriptor_CFLAGS = $(CHECK_CFLAGS)
test_reactor_descriptor_LDADD = $(CHECK_LDADD)
test_reactor_descriptor_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_descriptor_SOURCES = test/reactor_descriptor.c test/mock.c

check_PROGRAMS += test/reactor_pool
test_reactor_pool_CFLAGS = $(CHECK_CFLAGS)
test_reactor_pool_LDADD = $(CHECK_LDADD)
test_reactor_pool_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_pool_SOURCES = test/reactor_pool.c test/mock.c

check_PROGRAMS += test/reactor_timer
test_reactor_timer_CFLAGS = $(CHECK_CFLAGS)
test_reactor_timer_LDADD = $(CHECK_LDADD)
test_reactor_timer_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_timer_SOURCES = test/reactor_timer.c test/mock.c

check_PROGRAMS += test/reactor_resolver
test_reactor_resolver_CFLAGS = $(CHECK_CFLAGS)
test_reactor_resolver_LDADD = $(CHECK_LDADD)
test_reactor_resolver_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_resolver_SOURCES = test/reactor_resolver.c test/mock.c

check_PROGRAMS += test/reactor_stream
test_reactor_stream_CFLAGS = $(CHECK_CFLAGS)
test_reactor_stream_LDADD = $(CHECK_LDADD)
test_reactor_stream_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_stream_SOURCES = test/reactor_stream.c test/mock.c

check_PROGRAMS += test/reactor_tcp
test_reactor_tcp_CFLAGS = $(CHECK_CFLAGS)
test_reactor_tcp_LDADD = $(CHECK_LDADD)
test_reactor_tcp_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_tcp_SOURCES = test/reactor_tcp.c test/mock.c

check_PROGRAMS += test/reactor_http
test_reactor_http_CFLAGS = $(CHECK_CFLAGS) -DSRCDIR=\"$(srcdir)/\"
test_reactor_http_LDADD = $(CHECK_LDADD)
test_reactor_http_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_http_SOURCES = test/reactor_http.c test/mock.c

TESTS = $(check_PROGRAMS) test/valgrind.sh test/coverage.sh

## examples ###

bin_PROGRAMS = \
example/core \
example/tcp \
example/tcp_client \
example/http_server \
example/http_client \
example/http_proxy

example_core_LDFLAGS = -lreactor -ldynamic
example_tcp_LDFLAGS = -lreactor -ldynamic
example_tcp_client_LDFLAGS = -lreactor -ldynamic
example_http_server_LDFLAGS = -lreactor -ldynamic
example_http_client_LDFLAGS = -lreactor -ldynamic
example_http_proxy_LDFLAGS = -lreactor -ldynamic

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in libreactor-?.?.?.tar.gz
maintainer-clean-local:; rm -rf autotools m4 libreactor-?.?.?

CLEANFILES = {.,src/reactor,src/picohttpparser,test}/*.{gcno,gcda,gcov}
