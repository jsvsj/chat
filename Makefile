.PHONY:clean all
CC=g++
CFLAGS=-Wall -g 
BIN=charcli charsrv
all:$(BIN)
%.o:%.c
	$(CC) $(CFALGS) -c $< -o $@

clean:
	rm -f *.o $(BIN)
