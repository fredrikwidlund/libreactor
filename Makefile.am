AUTOMAKE_OPTIONS		= subdir-objects
ACLOCAL_AMFLAGS			= ${ACLOCAL_FLAGS} -I m4

DIST_SUBDIRS 			= example

SOURCE_FILES = \
src/picohttpparser/picohttpparser.c \
src/reactor/data.c \
src/reactor/pointer.c \
src/reactor/string.c \
src/reactor/list.c \
src/reactor/buffer.c \
src/reactor/vector.c \
src/reactor/hash.c \
src/reactor/map.c \
src/reactor/mapi.c \
src/reactor/maps.c \
src/reactor/utility.c \
\
src/reactor/reactor.c \
src/reactor/descriptor.c \
src/reactor/stream.c \
src/reactor/server.c \
src/reactor/net.c \
src/reactor/http.c \
src/reactor/timer.c \
src/reactor/notify.c

HEADER_FILES = \
src/picohttpparser/picohttpparser.h \
src/reactor/data.h \
src/reactor/pointer.h \
src/reactor/string.h\
src/reactor/list.h \
src/reactor/buffer.h \
src/reactor/vector.h \
src/reactor/hash.h \
src/reactor/map.h \
src/reactor/mapi.h \
src/reactor/maps.h \
src/reactor/utility.h \
\
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
libreactor_la_CFLAGS		= -std=gnu2x -g -O3 -flto -march=native -I$(srcdir)/src -fPIC
libreactor_la_LDFLAGS		= -static

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

AM_CFLAGS 			= -std=gnu2x -mno-sse -O0 -g -ftest-coverage -fprofile-arcs -I$(srcdir)/src -DGCOV_BUILD -DUNIT_TESTING
LDADD 				= -L. libreactor_test.a -lcmocka
#LDADD 				= -L. libreactor_test.a -lcmocka -lssl -lcrypto

check_LIBRARIES 		= libreactor_test.a
libreactor_test_a_SOURCES 	= $(SOURCE_FILES) $(HEADER_FILES)
libreactor_test_a_CFLAGS	= -$(AM_CFLAGS)

check_PROGRAMS 			= \
test/data \
test/pointer \
test/string \
test/list \
test/buffer \
test/vector \
test/hash \
test/map \
test/mapi \
test/maps \
test/utility

dist_noinst_SCRIPTS 		= test/valgrind.sh test/coverage.sh

TESTS 				= $(check_PROGRAMS) test/valgrind.sh test/coverage.sh
