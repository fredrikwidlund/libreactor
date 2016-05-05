ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -O3 -flto -fuse-linker-plugin
AM_LDFLAGS = -static

SOURCE_FILES = \
src/reactor_core/reactor_core.c \
src/reactor_core/reactor_desc.c \
src/reactor_core/reactor_signal.c \
src/reactor_core/reactor_signal_dispatcher.c \
src/reactor_core/reactor_stream.c \
src/reactor_core/reactor_timer.c \
src/reactor_core/reactor_user.c

HEADER_FILES = \
src/reactor_core/reactor_core.h \
src/reactor_core/reactor_desc.h \
src/reactor_core/reactor_signal.h \
src/reactor_core/reactor_signal_dispatcher.h \
src/reactor_core/reactor_stream.h \
src/reactor_core/reactor_timer.h \
src/reactor_core/reactor_user.h

MAIN_HEADER_FILES = \
src/reactor_core.h

AUTOMAKE_OPTIONS = subdir-objects
lib_LTLIBRARIES = libreactor_core.la
libreactor_core_la_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

headerfilesdir = $(includedir)/reactor_core
headerfiles_HEADERS = $(HEADER_FILES)

mainheaderdir = $(includedir)
mainheader_HEADERS = $(MAIN_HEADER_FILES)

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in libreactor_core-?.?.?.tar.gz
maintainer-clean-local:; rm -rf autotools m4 libreactor_core-?.?.?

### unit tests ###

CHECK_CFLAGS = -std=gnu11 -O0 -g -ftest-coverage -fprofile-arcs -I$(srcdir)/src
CHECK_LDADD = -L. -lreactor_core_test -ldynamic -lcmocka
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

check_LIBRARIES = libreactor_core_test.a
libreactor_core_test_a_CFLAGS = $(CHECK_CFLAGS)
libreactor_core_test_a_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

check_PROGRAMS = test/reactor_core
test_reactor_core_CFLAGS = $(CHECK_CFLAGS)
test_reactor_core_LDADD = $(CHECK_LDADD)
test_reactor_core_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_core_SOURCES = test/reactor_core.c test/stubs.c

check_PROGRAMS += test/reactor_desc
test_reactor_desc_CFLAGS = $(CHECK_CFLAGS)
test_reactor_desc_LDADD = $(CHECK_LDADD)
test_reactor_desc_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_desc_SOURCES = test/reactor_desc.c test/stubs.c

check_PROGRAMS += test/reactor_signal
test_reactor_signal_CFLAGS = $(CHECK_CFLAGS)
test_reactor_signal_LDADD = $(CHECK_LDADD)
test_reactor_signal_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_signal_SOURCES = test/reactor_signal.c test/stubs.c

check_PROGRAMS += test/reactor_stream
test_reactor_stream_CFLAGS = $(CHECK_CFLAGS)
test_reactor_stream_LDADD = $(CHECK_LDADD)
test_reactor_stream_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_stream_SOURCES = test/reactor_stream.c test/stubs.c

check_PROGRAMS += test/reactor_timer
test_reactor_timer_CFLAGS = $(CHECK_CFLAGS)
test_reactor_timer_LDADD = $(CHECK_LDADD)
test_reactor_timer_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_timer_SOURCES = test/reactor_timer.c test/stubs.c

check_PROGRAMS += test/reactor_user
test_reactor_user_CFLAGS = $(CHECK_CFLAGS)
test_reactor_user_LDADD = $(CHECK_LDADD)
test_reactor_user_LDFLAGS = $(CHECK_LDFLAGS_EXTRA)
test_reactor_user_SOURCES = test/reactor_user.c test/stubs.c

dist_noinst_SCRIPTS = test/valgrind.sh test/coverage.sh

TESTS = $(check_PROGRAMS) test/coverage.sh test/valgrind.sh

CLEANFILES = {.,src/reactor_core,test}/*.{gcno,gcda,gcov}
