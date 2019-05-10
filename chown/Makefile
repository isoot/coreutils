chown: chown.o savedir.o userspec.o
	gcc -o $@ $^

%.o:%.c
	gcc -c -o $@ $<

clean:
	rm -rf *.o chown
