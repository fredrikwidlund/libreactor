ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -g -O3 -flto -fuse-linker-plugin -I$(srcdir)/src
AM_LDFLAGS = -static

SOURCE_FILES = \
src/reactor/reactor_user.c \
src/reactor/reactor_desc.c \
src/reactor/reactor_core.c \
src/reactor/reactor_event.c \
src/reactor/reactor_timer.c \
src/reactor/reactor_stream.c \
src/reactor/reactor_tcp.c \
src/picohttpparser/picohttpparser.c \
src/reactor/reactor_http.c \
src/reactor/reactor_rest.c

HEADER_FILES = \
src/reactor/reactor_user.h \
src/reactor/reactor_desc.h \
src/reactor/reactor_core.h \
src/reactor/reactor_event.h \
src/reactor/reactor_timer.h \
src/reactor/reactor_stream.h \
src/reactor/reactor_tcp.h \
src/reactor/reactor_http.h \
src/reactor/reactor_rest.h

MAIN_HEADER_FILES = \
src/reactor.h

AUTOMAKE_OPTIONS = subdir-objects
lib_LTLIBRARIES = libreactor.la
libreactor_la_SOURCES = $(SOURCE_FILES) $(HEADER_FILES) src/picohttpparser/picohttpparser.h

headerfilesdir = $(includedir)/reactor
headerfiles_HEADERS = $(HEADER_FILES)

mainheaderdir = $(includedir)
mainheader_HEADERS = $(MAIN_HEADER_FILES)

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in libreactor-?.?.?.tar.gz
maintainer-clean-local:; rm -rf autotools m4 libreactor-?.?.?

## examples ###

bin_PROGRAMS = \
example/tcp_client \
example/tcp_server \
example/http_client \
example/http_server \
example/rest_server

example_tcp_client_LDADD = -L. -lreactor -ldynamic
example_tcp_server_LDADD = -L. -lreactor -ldynamic
example_http_client_LDADD = -L. -lreactor -ldynamic
example_http_server_LDADD = -L. -lreactor -ldynamic
example_rest_server_LDADD = -L. -lreactor -ldynamic

### unit tests ###

CHECK_CFLAGS = -std=gnu11 -O0 -g -ftest-coverage -fprofile-arcs -I$(srcdir)/src
CHECK_LDADD = -L. -lreactor_test -ldynamic -lcmocka
CHECK_LDFLAGS_EXTRA = \
-Wl,--wrap=malloc \
-Wl,--wrap=calloc \
-Wl,--wrap=realloc \
-Wl,--wrap=aligned_alloc \
-Wl,--wrap=epoll_create1 \
-Wl,--wrap=epoll_wait \
-Wl,--wrap=epoll_ctl \
-Wl,--wrap=fcntl \
-Wl,--wrap=read \
-Wl,--wrap=timerfd_create \
-Wl,--wrap=sigprocmask \
-Wl,--wrap=signalfd

check_LIBRARIES = libreactor_test.a
libreactor_test_a_CFLAGS = $(CHECK_CFLAGS)
libreactor_test_a_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

check_PROGRAMS = test/test_reactor_core
test_test_reactor_core_CFLAGS = $(CHECK_CFLAGS)
test_test_reactor_core_LDADD = $(CHECK_LDADD)
test_test_reactor_core_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_test_reactor_core_SOURCES = test/test_reactor_core.c test/stubs.c

dist_noinst_SCRIPTS = test/valgrind.sh test/coverage.sh

TESTS = $(check_PROGRAMS) test/coverage.sh test/valgrind.sh

CLEANFILES = {.,src/reactor,test}/*.{gcno,gcda,gcov}
