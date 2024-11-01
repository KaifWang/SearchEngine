CC=g++ -std=c++11

all: indexServer index

indexServer: indexServer.cpp ../queryParser/queryParser.o ../queryParser/expression.o ../queryParser/tokenstream.o
	$(CC) $^ -o indexServer -pthread -DDEBUG -g

index: IndexBuilder.cpp index.h HashBlob.h PostingList.h DeltaConverter.h ../utility/HashTable.h 
	$(CC) IndexBuilder.cpp -o buildIndex

clean: 
	rm -f buildIndex
	rm -f indexServer
	rm -f hashfile*

.PHONY: clean
