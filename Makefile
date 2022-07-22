CC = gcc
CFLAGS = -ansi -Wall -pedantic

all: ejecutable

ejecutable: main.o misc.o sequence.o ordenaproc.o ordenaproc_child.c pipe_utils.o
	$(CC) $(CFLAGS) -o ordenaproc main.o misc.o sequence.o ordenaproc.o ordenaproc_child.c pipe_utils.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

ordenaproc.o: ordenaproc.c
	$(CC) $(CFLAGS) -c ordenaproc.c

ordenaproc_child.o: ordenaproc_child.c
	$(CC) $(CFLAGS) -c ordenaproc_child.c

pipe_utils.o: pipe_utils.c
	$(CC) $(CFLAGS) -c pipe_utils.c

sequence.o: sequence.c
	$(CC) $(CFLAGS) -c sequence.c

misc.o: misc.c
	$(CC) $(CFLAGS) -c misc.c

clean:
	rm *.o ordenaproc ordenahilo ordena