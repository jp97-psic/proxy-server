CXX = g++
CXXFLAGS = -g -Wall -Wextra -pedantic -O0 $(GXX_FLAGS)
CXXFLAGS += -std=c++11
LFLAGS = -lssl -lcrypto -fexceptions

DEP_FLAGS = -MMD -MP

CXXFLAGS += $(DEP_FLAGS)

SRC = $(wildcard *.cpp)
HDR = $(wildcard *.h)
OBJ = $(SRC:.cpp=.o)
DEP = $(SRC:.cpp=.d)

APP = server.exe
ZIP = $(APP).tgz

all: $(APP)

rebuild: clean $(APP)

$(APP): $(OBJ)
	$(CXX) $(LFLAGS) $^ -o $@ 

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

zip: $(SRC) $(HDR)
	tar czf $(ZIP) $^ makefile

run: $(APP)
	@./$(APP)

crun: $(APP)
	@clear
	@./$(APP)

valgrind: $(APP)
	@valgrind --leak-check=full ./$(APP)

gdb: $(APP)
	@gdb ./$(APP)

clean: 
	@rm -f $(APP) $(OBJ) $(DEP)

.PHONY: all rebuild zip run crun valgrind gdb clean plot

-include $(DEP)