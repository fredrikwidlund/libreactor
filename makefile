PROG	= reactor
CFLAGS	= -g -Wall -Wpedantic -Werror -O3 -std=gnu11
OBJS	= main.o reactor.o reactor_fd.o reactor_signal.o reactor_timer.o reactor_socket.o

.PHONY: clean

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(PROG) $(OBJS)
