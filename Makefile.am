LIB_CFLAGS = -O3 -std=gnu11

LIB_TEST_CFLAGS = -O0 -g -ftest-coverage -fprofile-arcs -std=gnu11 -I/opt/local/include -Isrc
#LDADD_TEST_FLAGS = -L. -lcmocka

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

SOURCE_FILES = src/buffer.c src/reactor.c src/reactor_client.c src/reactor_fd.c src/reactor_resolver.c src/reactor_signal.c \
	src/reactor_signal_dispatcher.c src/reactor_socket.c src/reactor_timer.c src/vector.c
HEADER_FILES = src/buffer.h src/reactor.h src/reactor_client.h src/reactor_fd.h src/reactor_resolver.h src/reactor_signal.h \
	src/reactor_signal_dispatcher.h src/reactor_socket.h src/reactor_timer.h src/vector.h

lib_LTLIBRARIES = libreactor.la
libreactor_la_CFLAGS = $(LIB_CFLAGS)
libreactor_la_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

check_LIBRARIES = libreactor_test.a
libreactor_test_a_CFLAGS = $(LIB_TEST_CFLAGS)
libreactor_test_a_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

BUILD_TESTS = test/test1 test/test2 test/test3 test/test4
TESTS = $(BUILD_TESTS) test/test_valgrind.sh

check_PROGRAMS = $(BUILD_TESTS)
check_SCRIPTS = test
TEST_LD_WRAP = -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=aligned_alloc

AM_CFLAGS = $(LIB_TEST_CFLAGS)
AM_LDFLAGS = $(LDADD_TEST_FLAGS) $(TEST_LD_WRAP) -L. -lreactor_test -lanl

test_test1_SOURCES = test/test1.c test/test_util.c
test_test2_SOURCES = test/test2.c test/test_util.c
test_test3_SOURCES = test/test3.c test/test_util.c
test_test4_SOURCES = test/test4.c test/test_util.c

headerfilesdir = $(includedir)/reactor
headerfiles_HEADERS = $(HEADER_FILES)

mainheaderdir = $(includedir)
mainheader_HEADERS = src/reactor.h

#dist_man3_MANS = man/reactor.3

dist_noinst_SCRIPTS = autogen.sh test/test_valgrind.sh

CLEANFILES = {./,src,test}/*.{gcno,gcda,gcov}
MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in libreactor-*.tar.gz INSTALL

maintainer-clean-local:
	rm -rf autotools m4 libreactor-0.1.0
