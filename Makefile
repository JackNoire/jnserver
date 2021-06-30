HDRS = handle_request.h
CC = gcc
CFLAGS = -O2 -Wno-unused-result
OBJS = main.o handle_request.o

all: jnserver

jnserver: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c $(HDRS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm *.o jnserver