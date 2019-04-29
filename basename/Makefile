basename: bname.o strip.o basename.o
	gcc -o $@ $^

%.o : %.c
	gcc -c -o $@ $<

clean:
	rm -f *.o basename
