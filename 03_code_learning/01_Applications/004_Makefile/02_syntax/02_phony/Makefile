# .PHONY演示

test:a.o b.o c.o
	gcc -o test a.o b.o c.o
%.o : %.c
	gcc -c -o $@ $<
clean:
	rm ./*.o test

.PHONY: clean