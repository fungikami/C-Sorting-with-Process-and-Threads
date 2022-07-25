CC = gcc
CFLAGS = -ansi -Wall -pedantic

all: ordenaproc ordenahilo ordena

ordenaproc: ordenaproc.o ordenaproc_child.c pipe_utils.o sequence.o misc.o
	$(CC) $(CFLAGS) -o ordenaproc ordenaproc.o ordenaproc_child.c pipe_utils.o sequence.o misc.o

ordenahilo: ordenahilo.o sequence.o misc.o
	$(CC) $(CFLAGS) -o ordenahilo ordenahilo.o sequence.o misc.o

ordena: ordena.o sequence.o misc.o
	$(CC) $(CFLAGS) -o ordena ordena.o sequence.o misc.o

ordenaproc.o: ordenaproc.c
	$(CC) $(CFLAGS) -c ordenaproc.c

ordenaproc_child.o: ordenaproc_child.c
	$(CC) $(CFLAGS) -c ordenaproc_child.c

pipe_utils.o: pipe_utils.c
	$(CC) $(CFLAGS) -c pipe_utils.c

ordenahilo.o: ordenahilo.c
	$(CC) $(CFLAGS) -c ordenahilo.c

ordena.o: ordena.c
	$(CC) $(CFLAGS) -c ordena.c

sequence.o: sequence.c
	$(CC) $(CFLAGS) -c sequence.c

misc.o: misc.c
	$(CC) $(CFLAGS) -c misc.c

clean:
	rm *.o ordenaproc ordenahilo ordena