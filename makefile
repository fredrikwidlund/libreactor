PROG	= reactor
CFLAGS	= -g -Wall -Wpedantic -Werror -O3 -std=gnu11
LDADD	= -lanl
OBJS	= main.o reactor.o reactor_fd.o reactor_signal.o reactor_timer.o reactor_socket.o reactor_signal_dispatcher.o reactor_resolver.o

.PHONY: clean

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDADD)

clean:
	rm -f $(PROG) $(OBJS)
