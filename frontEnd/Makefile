CC=g++ -g -std=c++11

all: webServer

webServer: webServer.cpp ../queryParser/queryParser.o ../queryParser/expression.o ../queryParser/tokenstream.o
	$(CC) $^ -o webServer -pthread

clean: 
	rm -f localServer
	rm -f webServer

.PHONY: clean
