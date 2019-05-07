LIB_SRC_s = $(wildcard ./Source/server/*.c)
LIB_OBJ_s = $(LIB_SRC_s:.c=.o)
LIB_SRC_c = $(wildcard ./Source/sh13/*.c)
LIB_OBJ_c = $(LIB_SRC_c:.c=.o)
CC = gcc
FLAGS = -Wall  -I/usr/include/SDL2 -lSDL2_image -lSDL2_ttf -lSDL2 -lpthread -o
all: server sh13

server: $(LIB_SRC_s)
	$(CC) $(LIB_SRC_s) $(FLAGS)     $@
sh13: $(LIB_SRC_c)
	$(CC) $(LIB_SRC_c)  $(FLAGS)    $@
clean:
	rm -rf *.o server sh13
generate:
	./server localhost 32000 
.depend:
	$(CC) -MM $(LIB_SRC_c) $(LIB_SRC_s) > .depend ;

-include .depend

.PHONY: clean all
