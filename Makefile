CC = gcc
CFLAGS = -ansi -Wall -pedantic

all: ejecutable

ejecutable: main.o misc.o ordenaproc.o
	$(CC) $(CFLAGS) -o ordenaproc main.o misc.o ordenaproc.o
	$(CC) $(CFLAGS) -o ordenahilo main.o misc.o ordenaproc.o
	$(CC) $(CFLAGS) -o ordena main.o misc.o ordenaproc.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

ordenaproc.o: ordenaproc.c
	$(CC) $(CFLAGS) -c ordenaproc.c

misc.o: misc.c
	$(CC) $(CFLAGS) -c misc.c

clean:
	rm *.o ordenaproc ordenahilo ordena