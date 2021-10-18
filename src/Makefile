CC=gcc
OPTS=-g -std=c99 -Werror

all: main.o predictor.o gshare.o local.o
	$(CC) $(OPTS) -lm -o predictor main.o predictor.o gshare.o local.o

main.o: main.c predictor.h
	$(CC) $(OPTS) -c main.c

predictor.o: predictor.h predictor.c
	$(CC) $(OPTS) -c predictor.c

gshare.o: gshare.h gshare.c
	$(CC) $(OPTS) -c gshare.c

local.o: local.h local.c
	$(CC) $(OPTS) -c local.c

clean:
	rm -f *.o predictor;
