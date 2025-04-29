CFLAGS = -g -Wall

gthr_demo: g_queue.o g_semaphore.o gthr_switch.o gthr.o main.o
	$(CC) -o $@ $^ -lm

.S.o:
	as -o $@ $^

.PHONY: clean
clean:
	rm -f *.o gthr_demo
