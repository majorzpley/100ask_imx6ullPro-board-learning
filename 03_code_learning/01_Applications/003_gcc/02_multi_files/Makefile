GCC_PREFIX = 
INCLUDE_PATH = ./
src  := $(shell ls *.c)
objs := $(patsubst %.c,%.o,$(src))

test: $(objs)
	$(GCC_PREFIX)gcc -I$(INCLUDE_PATH) -o $@ $^

%.o:%.c
	$(GCC_PREFIX)gcc -I$(INCLUDE_PATH) -c -o $@ $<

clean:
	rm -f test *.o		