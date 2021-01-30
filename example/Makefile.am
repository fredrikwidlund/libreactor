ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -I../src
LDADD = -L.. -lreactor -ldynamic

bin_PROGRAMS = \
timer \
stream \
notify \
httpd_lowlevel \
server \
server_single \
benchmark_http \
resolve_sync \
resolve_async

resolve_async_LDFLAGS = -pthread

MAINTAINERCLEANFILES = Makefile.in
