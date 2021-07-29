
CC := g++
SRC := $(wildcard *.cpp)
OBJ := $(SRC:%.cpp=%.o)
HEADER := $(wildcard %.h)



app:$(OBJ)
	$(CC) -pthread $(OBJ) -o $@ $(addprefix -I, $(HEADER))
%.o:%.cpp
	$(CC) $< -c  


.PHONY:clean

clean:
	rm app *.o
