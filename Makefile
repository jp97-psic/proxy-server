CXX=g++
CXXFLAGS = -g -Wall -std=c++17

SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)
DEP = $(SRC:.cpp=.d)

DEP_FLAGS=-MMD -MP
CXXFLAGS+=$(DEP_FLAGS)

all: server.exe

server.exe: HTTPServer.o server.o
	$(CXX) $(CXXFLAGS) $^ -o $@

run: server.exe
	./server.exe

val: server.exe
	valgrind --leak-check=full ./server.exe

clean:
	rm -f *.o *.d server.exe

.PHONY: all run val clean

-include $(DEP)
