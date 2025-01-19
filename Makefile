CFLAGS = -std=c11 -g -fno-common
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

dcc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): dcc.h

test: dcc
	@./test.sh

clean:
	$(RM) -f dcc *.o *~ tmp*

.PHONY: test clean

