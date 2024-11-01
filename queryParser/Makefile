CC=g++ -g -std=c++11

all: queryParser.o expression.o tokenstream.o

queryParser.o: queryParser.cpp
	$(CC) -c $*.cpp -o $*.o

tokenstream.o: tokenstream.cpp
	$(CC) -c $*.cpp -o $*.o

expression.o: expression.cpp
	$(CC) -c $*.cpp -o $*.o

clean: 
	rm -f *.o

.PHONY: clean
