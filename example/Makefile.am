ACLOCAL_AMFLAGS		= ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS		= -I../src -O3 -flto -march=native -static
LDADD			= -L.. -lreactor

bin_PROGRAMS 		= \
server \
tls-server \
timer \
notify \
request \
responder \
responder3

responder_LDADD 	= $(LDADD) -lssl -lcrypto
responder3_LDADD 	= $(LDADD) -lssl -lcrypto
request_LDADD 		= $(LDADD) -lssl -lcrypto
server_LDADD 		= $(LDADD) -lssl -lcrypto
tls_server_LDADD 	= $(LDADD) -lssl -lcrypto

MAINTAINERCLEANFILES 	= Makefile.in
