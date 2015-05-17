CFLAGS	= -g -Wall -Wpedantic -Werror -O3 -std=gnu11
LDADD	= -lanl
OBJS	= buffer.o vector.o reactor.o reactor_fd.o reactor_signal.o reactor_timer.o reactor_socket.o reactor_signal_dispatcher.o reactor_resolver.o reactor_client.o

.PHONY: clean

all: test1 test2 test3 test4

test1: test1.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDADD)

test2: test2.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDADD)

test3: test3.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDADD)

test4: test4.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDADD)

clean:
	rm -f test1 test1.o test2 test2.o test3 test3.o test4 test4.o $(OBJS)
