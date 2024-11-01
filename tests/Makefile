CC=g++ -g -std=c++11

all: testEncodeDecode testQuery
	
testSystem: testSystem.cpp
	$(CC) testSystem.cpp -o testSystem

testSystem2: testSystem2.cpp
	$(CC) testSystem2.cpp -o testSystem2

testISR: testISR.cpp
	$(CC) testISR.cpp -o testISR

testConverter: testConverter.cpp DeltaConverter.h
	$(CC) testConverter.cpp -o testConverter

testRanker: testRanker.cpp 
	$(CC) testRanker.cpp -o testRanker

testQuery: testQuery.cpp ../queryParser/queryParser.o ../queryParser/expression.o ../queryParser/tokenstream.o
	$(CC) $^ -DDEBUG -o testQuery

testEncodeDecode: testEncodeDecode.cpp
	$(CC) testEncodeDecode.cpp -o testEncodeDecode

clean: 
	rm -f systemTest systemTest2 testISR testConverter 
	rm -f testRanker testQuery testEncodeDecode
.PHONY: clean
