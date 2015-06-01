LIB_CFLAGS = -O3 -std=gnu11 -g

LIB_TEST_CFLAGS = -O0 -g -ftest-coverage -fprofile-arcs -std=gnu11 -I/opt/local/include -Isrc

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

SOURCE_FILES = src/buffer.c src/vector.c src/reactor_core.c src/reactor_fd.c
#src/reactor_client.c src/reactor_fd.c src/reactor_resolver.c src/reactor_signal.c \
#	src/reactor_signal_dispatcher.c src/reactor_socket.c src/reactor_timer.c src/vector.c
HEADER_FILES = src/buffer.h src/vector.h src/reactor_core.h src/reactor_fd.h
#src/reactor_client.h src/reactor_fd.h src/reactor_resolver.h src/reactor_signal.h \
#	src/reactor_signal_dispatcher.h src/reactor_socket.h src/reactor_timer.h src/vector.h

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

test_test1_CFLAGS = $(LIB_TEST_CFLAGS)
test_test1_LDFLAGS = -L. -lreactor_test -lanl

test_test2_CFLAGS = $(LIB_TEST_CFLAGS)
test_test2_LDFLAGS = -L. -lreactor_test -lanl

test_test3_CFLAGS = $(LIB_TEST_CFLAGS)
test_test3_LDFLAGS = -L. -lreactor_test -lanl

test_test4_CFLAGS = $(LIB_TEST_CFLAGS)
test_test4_LDFLAGS = -L. -lreactor_test -lanl

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
