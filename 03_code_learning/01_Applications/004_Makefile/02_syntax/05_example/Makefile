# 演示
objs = a.o b.o c.o

depend_files := $(patsubst %,.%.d,$(objs))
depend_files := $(wildcard $(depend_files))

CFLAGS = -Werror -Iinclude

test : $(objs)
	gcc -o $@ $^

ifneq ($(depend_files), )
include $(depend_files)
endif


%.o : %.c
	gcc $(CFLAGS) -c -o $@ $< -MD -MF .$@.d

clean:
	rm *.o test

distclean:
	rm $(depend_files)

.PHONY: clean 