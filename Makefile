CFLAGS = -std=c11 -g -fno-common

dcc: main.o
	$(CC) -o $@ $^ $(LDFLAGS)

test: dcc
	@./test.sh

clean:
	$(RM) -f dcc *.o *~ tmp*

.PHONY: test clean

