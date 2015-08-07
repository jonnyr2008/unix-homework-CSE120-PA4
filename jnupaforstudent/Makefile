# Makefile to compile Umix Programming Assignment 4 (pa4) 

LIBDIR = $(UMIXPUBDIR)/lib
# LIBDIR = $(UMIXROOTDIR)/sys

CC 	= cc 
#FLAGS 	= -g -L$(LIBDIR) -lumix4
#FLAGS 	= -g -L$(LIBDIR) -lumix4

PA4 =	pa4a pa4b pa4c

pa4:	$(PA4)

pa4a:	pa4a.c auy.h
	$(CC) $(FLAGS) -o pa4a pa4a.c

pa4b:	pa4b.c auy.h  mythreads.h mythreads.o
	$(CC) $(FLAGS) -o pa4b pa4b.c mythreads.o

pa4c:	pa4c.c auy.h  mythreads.h mythreads.o
	$(CC) $(FLAGS) -o pa4c pa4c.c mythreads.o

mythreads.o:	mythreads.c auy.h mythreads.h
	$(CC) $(FLAGS) -c mythreads.c

clean:
	rm -f *.o $(PA4)

