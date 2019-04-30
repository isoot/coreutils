chmod:chmod.o filemode.o modechange.o savedir.o
	gcc -o $@ $^

%.o : %.c
	gcc -c -o $@ $<

clean:
	rm -f *.o chmod
