# Search Us -- System Design of a Search Engine
The main components include: crawler (including an HTML parser), index, constraint solver, query parser, ranker, and front end.

## 1. Runnign the Crawler
- Depoly crawler code on 10 aws machines (from machine0 to machine9)
- Add all the machine IP addresses to crawler/configs/machine_hostname
- Starting the crawler on all 10 machines
```
make all
./crawler.o 0 > output.txt &
./crawler.o 1 > output.txt &
...
./crawler.o 9 > output.txt &
```
where the argument is the machine number in which the crawler is run.
- Activate auto reboot script upon crawler failure
```
./start0
./start1
...
./start9
```
- The crawler stores all the parsed html under crawler/outputs/parsed_output/

## 2. File format for each parsed HTML page in parsed_output/
```
url
```
```
numTitleWords
```
```
numBodyWords
```
```
#titleWord1 (decorated)
```
```
...
```
```
#titleWordn (decorated)
```
```
BodyWord1
```
```
...
```
```
BodyWordn
```

## 3. Build Inverted Index
- Needs to build query parser first as a dependecy for index building
```
cd queryparser
make clean
make
cd ..
```

- **Move all parsed HTML pages from crawler to index/parsed_output**
- Build the index
```
cd index
make clean
make
./buildIndex
```
It will build index chunks to index/indexChunks/ folder
- Move all index chunks to completeIndex/ for next step

## 4. Deploy Index Servers and Front End Server
- Deploy index code to 10 aws machines
- Deploy front end code to 1 other aws machines
- Put the front end IP in configs/config.h
- Add all index server machine IPs to frontEnd/IndexChunkIp.txt
- Go to each index machine, then
- Needs to build query parser first as a dependecy for the index server
```
cd queryparser
make clean
make
cd ..
```

- Starting index server
```
cd index
make clean
make
./indexServer
```
- Go to the front end machine
- Needs to build query parser first as a dependecy for the front end
```
cd queryparser
make clean
make
cd ..
```
- starting front end server
```
cd frontEnd
make clean
make
./webServer 8000 $(pwd)/static
```
- **Navigate to [Front end IP]:8000/index.html on the browser to search**
## 5. Running Local Test
```
Note: most programs in tests require running "make" in queryParser
directory first for required dependencies. 
```
### Test index and search without servers running
- The query being searched can be modified in the main function of queryParser/testQuery.cpp
```
cd tests
make clean
make testQuery
./testQuery
```
## 6. Understanding configs/config.h
### **Crawler configs**
Machine_hostname.txt contains all the public DNS name for each aws machine. Top is machine 0, and bottom is machin 9. 

UMSE_CRAWLER_NUM_CONCURRENT defines how many concurrent threads in the downloading and parsing phase.

UMSE_CRAWLER_NUM_MACHINE defines how many machines there are, need to be consistent with the machine_hostname.txt file. 

UMSE_CRAWLER_FRONTIER_MAX_SIZE defines the maximum frontier list size. 

All other configs are in the file crawler/configs/crawler_configs. 
### **Machine communication configs and frontEnd configs**
INDEX_IP_FILE is the file in frontEnd directory that lists IP address for all the machines 
where index servers are run.

FRONT_END_IP is where we hardcode the IP address of the machine that hosts the frontEnd
server.

### **Index configs**
NUM_DOCS_IN_DICT is the number of documents (parser outputs) we store in our index chunk
before serializing the index chunk to be stored in a binary file on disk.

INPUT_DIRECTORY is the directory that the HTML parser stores its output files to and is read
by the indexBuilder

INDEX_DIRECTORY is the directory we store binary files to while building our index

FINAL_INDEX is the directory that stores all the index chunks after they are built. It's also the
directory used by our final ranker.

## 7. Build 
### Built With
g++ -std=c++11
### Debug with
g++ -g -std=c++11
### Turn on Ranker Scoring info
g++ -g -DDEBUG -std=c++11

## Authors 
Zhicheng Huang, Xiao Song, Patrick Su, Kai Wang, Leo Yan, Wenxuan Zhao

## Acknowledgments
* Some helper function in webServer.cpp are created by Nicole Hamilton as the starter code
for hw10 LinuxTinyServer.

