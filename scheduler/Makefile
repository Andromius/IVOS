CFLAGS = -g -Wall

gthr_demo: gthr.o gthr_switch.o main.o
	$(CC) -o $@ $^ -lm

.S.o:
	as -o $@ $^

.PHONY: clean
clean:
	rm -f *.o gthr_demo
