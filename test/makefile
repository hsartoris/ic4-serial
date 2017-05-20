#
# Makefile for MacOS X
#
C =		gcc -c -DUNIX -DMACOSX
L =		gcc
LIBS =		-ldl

all:  		ismain

ismain:		main.o isense.o
		$(L) -o $@ main.o isense.o $(LIBS)

main.o:		main.c *.h
		$(C) main.c

isense.o:	isense.c *.h
		$(C) isense.c

clean:
	  rm -f *.o ismain
