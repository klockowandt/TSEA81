CFLAGS=-O2 -g -Wall
LIBS=-lrt -lpthread
CFILES=lift.c main.c messages.c
HEADERS=lift.h messages.h
lift_messages: $(CFILES) $(HEADERS)
	gcc $(CFLAGS) $(CFILES) -o lift_messages $(LIBS) 

clean:
	rm -f lift_messages *~ *.o
