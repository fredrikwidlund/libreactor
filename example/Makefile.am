ACLOCAL_AMFLAGS		= ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS		= -I../src -O3 -flto -march=native -static
AM_LDFLAGS		= -Wl,--gc-sections -fdata-sections -ffunction-sections
LDADD			= -L.. -lreactor

bin_PROGRAMS 		= \
fd \
descriptor \
stream \
timer \
notify \
responder \
string \
http-performance \
server \
tls-server \
resolver

responder_LDADD		= $(LDADD) -lssl -lcrypto
tls_server_LDADD	= $(LDADD) -lssl -lcrypto
resolver_LDFLAGS        = $(LDADD) -pthread

MAINTAINERCLEANFILES 	= Makefile.in
