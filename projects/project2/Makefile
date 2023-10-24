BIN = ./bin
OBJS	= $(BIN)/main.o $(BIN)/sleep.o
SOURCE	= main.cpp sleep.cpp
HEADER	= sleep.hh
OUT	= simulation
CC	 = g++
FLAGS	 = -c -Wno-error
LFLAGS	 = -lpthread -pthread

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

$(BIN)/main.o: main.cpp
	$(CC) $(FLAGS) main.cpp -std=c++11 -o $(BIN)/main.o

$(BIN)/sleep.o: sleep.cpp
	$(CC) $(FLAGS) sleep.cpp -std=c++11 -o $(BIN)/sleep.o

clean:
	rm -f $(OBJS) $(OUT)