src := $(wildcard *.c)
obj := $(src:%.c=%.o)
CC=gcc
CFLAGS=-Wall # -DDEBUG -g
CLIBS=-lm -lcrypto

target := opsim

default: $(target)

$(obj): %.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(target): $(obj)
	$(CC) $(CFLAGS) $(CLIBS) -o $@ $^

clean:
	rm -f $(obj) $(target) *~

.PHONY: clean
