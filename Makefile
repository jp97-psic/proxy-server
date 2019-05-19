CXX=g++
CXXFLAGS = -g -Wall -std=c++17

SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)
DEP = $(SRC:.cpp=.d)

DEP_FLAGS=-MMD -MP
CXXFLAGS+=$(DEP_FLAGS)

all: server

server: HTTPServer.o server.o
	$(CXX) $(CXXFLAGS) $^ -o $@

run: server
	./server

val: server
	valgrind --leak-check=full ./server

clean:
	rm -f *.o *.d server

.PHONY: all run val clean

-include $(DEP)
