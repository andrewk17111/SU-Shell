CFLAGS=-ggdb 
LDFLAGS=-ggdb

all: sushell

# .c --> .o file
.c.o:
	gcc $(CFLAGS) -c $<

# .o --> executable file
parser: parser.o
	gcc -o parser list.c $< $(LDFLAGS) 

sushell: sush.o
	gcc -o sush runner.c parser.c list.c environ.c internal.c $< $(LDFLAGS) 

run: sush
	./sush

valgrind: sush
	valgrind --leak-check=full ./sushell

clean:
	rm sush *.o
	rm -fr *.dSYM