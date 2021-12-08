AUTOMAKE_OPTIONS		= subdir-objects
ACLOCAL_AMFLAGS			= ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS			= -std=gnu2x -g -O3 -flto -march=native -I$(srcdir)/src -fPIC
AM_LDFLAGS			= -static

DIST_SUBDIRS 			= example

SOURCE_FILES = \
src/reactor/string.c \
src/reactor/buffer.c \
src/reactor/vector.c \
src/reactor/list.c \
src/reactor/utility.c \
src/picohttpparser/picohttpparser.c \
src/reactor/reactor.c \
src/reactor/descriptor.c \
src/reactor/stream.c \
src/reactor/server.c \
src/reactor/net.c \
src/reactor/http.c \
src/reactor/timer.c \
src/reactor/notify.c

HEADER_FILES = \
src/reactor/string.h \
src/reactor/buffer.h \
src/reactor/vector.h \
src/reactor/list.h \
src/reactor/utility.h \
src/picohttpparser/picohttpparser.h \
src/reactor/reactor.h \
src/reactor/descriptor.h \
src/reactor/stream.h \
src/reactor/server.h \
src/reactor/net.h \
src/reactor/http.h \
src/reactor/timer.h \
src/reactor/notify.h

lib_LTLIBRARIES 		= libreactor.la
libreactor_la_SOURCES 		= $(SOURCE_FILES) $(HEADER_FILES)

headerfilesdir 			= $(includedir)/reactor
headerfiles_HEADERS 		= $(HEADER_FILES)

mainheaderdir 			= $(includedir)
mainheader_HEADERS 		= src/reactor.h

pkgconfigdir 			= $(libdir)/pkgconfig
pkgconfig_DATA 			= libreactor.pc

MAINTAINERCLEANFILES 		= aclocal.m4 config.h.in configure Makefile.in libreactor-?.?.?.tar.gz

CLEANFILES 			= {.,test/,src/reactor/,src/picohttpparser}/*.{gcno,gcda,gcov}

maintainer-clean-local:
	rm -rf autotools m4 libreactor-?.?.?

indent:
	clang-format -i src/reactor/*.c test/*.c

### unit testing ###

check_LIBRARIES 		= libreactor_test.a
libreactor_test_a_CFLAGS 	= $(CHECK_CFLAGS)
libreactor_test_a_SOURCES 	= $(SOURCE_FILES) $(HEADER_FILES)

CHECK_CFLAGS 			= -std=gnu2x -mno-sse -O0 -g -ftest-coverage -fprofile-arcs -I$(srcdir)/src -DGCOV_BUILD -DUNIT_TESTING
CHECK_LDADD 			= -L. libreactor_test.a -lcmocka -lssl -lcrypto

check_PROGRAMS 			=

check_PROGRAMS 			+= test/reactor
test_reactor_CFLAGS 		= $(CHECK_CFLAGS)
test_reactor_LDADD 		= $(CHECK_LDADD)
test_reactor_LDFLAGS 		= $(CHECK_LDFLAGS_EXTRA) -Wl,--wrap=epoll_wait
test_reactor_SOURCES 		= test/reactor.c test/mock.c

check_PROGRAMS 			+= test/descriptor
test_descriptor_CFLAGS 		= $(CHECK_CFLAGS)
test_descriptor_LDADD 		= $(CHECK_LDADD)
test_descriptor_LDFLAGS 	= $(CHECK_LDFLAGS_EXTRA)
test_descriptor_SOURCES 	= test/descriptor.c

check_PROGRAMS 			+= test/stream
test_stream_CFLAGS 		= $(CHECK_CFLAGS)
test_stream_LDADD 		= $(CHECK_LDADD)
test_stream_LDFLAGS 		= $(CHECK_LDFLAGS_EXTRA)
test_stream_SOURCES 		= test/stream.c

check_PROGRAMS 			+= test/server
test_server_CFLAGS		= $(CHECK_CFLAGS)
test_server_LDADD 		= $(CHECK_LDADD)
test_server_LDFLAGS		= $(CHECK_LDFLAGS_EXTRA)
test_server_SOURCES		= test/server.c

check_PROGRAMS 			+= test/net
test_net_CFLAGS			= $(CHECK_CFLAGS)
test_net_LDADD 			= $(CHECK_LDADD)
test_net_LDFLAGS		= $(CHECK_LDFLAGS_EXTRA)
test_net_SOURCES		= test/net.c

check_PROGRAMS 			+= test/timer
test_timer_CFLAGS		= $(CHECK_CFLAGS)
test_timer_LDADD 		= $(CHECK_LDADD)
test_timer_LDFLAGS		= $(CHECK_LDFLAGS_EXTRA)
test_timer_SOURCES		= test/timer.c

check_PROGRAMS 			+= test/notify
test_notify_CFLAGS		= $(CHECK_CFLAGS)
test_notify_LDADD 		= $(CHECK_LDADD)
test_notify_LDFLAGS		= $(CHECK_LDFLAGS_EXTRA)
test_notify_SOURCES		= test/notify.c

dist_noinst_SCRIPTS 		= test/valgrind.sh test/coverage.sh

TESTS 				= $(check_PROGRAMS) test/valgrind.sh test/coverage.sh
