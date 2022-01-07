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
server-ssl

responder_LDADD		= $(LDADD) -lssl -lcrypto
server_ssl_LDADD	= $(LDADD) -lssl -lcrypto

MAINTAINERCLEANFILES 	= Makefile.in
