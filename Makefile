CFLAGS=-ggdb 
LDFLAGS=-ggdb

all: sushell

# .c --> .o file
.c.o:
	gcc $(CFLAGS) -c $<

# .o --> executable file
parser: parser.o
	gcc -o parser list.c $< $(LDFLAGS) 

sushell: sushell.o
	gcc -o sushell runner.c parser.c list.c $< $(LDFLAGS) 

run: sushell
	./sushell

clean:
	rm sushell *.o 
	rm -fr *.dSYM